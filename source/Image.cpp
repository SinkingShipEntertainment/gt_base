
#include "gt_base/Image.h"
#include "gt_base/types/f32x2.h"
#include "gt_base/Logger.h"
#include "gt_base/string.h"
#include "gt_base/globals.h"
#if USING_VULKAN
  #include "RenderUtils.h"
#endif

#ifdef SSE_CENTOS7
  #include <setjmp.h>
//   #include <png.h> 
#else
  #include <tiff/tiffio.h>
  #include <hdrloader.h>
#endif

#include <jpeglib.h>
#include <spng.h>
#include <tga.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfStringAttribute.h>
#include <OpenEXR/ImfFloatAttribute.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfFrameBuffer.h>

#if defined(__GNUC__) && __GNUC__ < 8
  #include <experimental/filesystem>
  namespace filesystem = std::experimental::filesystem;
#else
  #include <filesystem> /// C++17 
#endif
// #include <boost/filesystem.hpp>  

#include <stdexcept>
#include <array>
#include <algorithm>

using namespace gt;
using namespace std;


Image::Image()
    : width(0)
      ,height(0)
      ,numComps(0)
      ,bytesPerComp(1)
      ,type(Image::UNDEFINED)
      ,pitch(0)
      ,bytesTotal(0)
      ,data(nullptr)
      ,inChannelMode(false)
      ,isCubeMap(false)
      ,numMipMaps(1)
      ,inGPU(false)
#if USING_VULKAN
      ,vkFormat(VK_FORMAT_UNDEFINED)
      ,_isDepthBuffer(false)
#endif
{
  this->tally();
}

Image::Image(u32 w, u32 h, u8 numComps, u8 bytesPerComp, u8 type, bool isDepthBuffer)
    : width(w)
      ,height(h)
      ,numComps(numComps)
      ,bytesPerComp(bytesPerComp)
      ,type(type)
      ,data(nullptr)
      ,inChannelMode(false)
      ,isCubeMap(false)
      ,numMipMaps(1)
      ,inGPU(false)
#if USING_VULKAN
      ,vkFormat(VK_FORMAT_UNDEFINED)
      ,_isDepthBuffer(isDepthBuffer)
#endif
{
  this->alloc();
}

/// attrNames: attributes you're interested in retrieving from the header if they exists, to be accessed with metaAttrs
Image::Image(string const & filepath, vector<string> attrNames, vector<string> channelFilter)
    : width(0)
      ,height(0)
      ,numComps(0)
      ,bytesPerComp(0)
      ,type(Image::UNDEFINED)
      ,data(nullptr)
      ,inChannelMode(false)
      ,isCubeMap(false)
      ,numMipMaps(1)
      ,inGPU(false)
#if USING_VULKAN
      ,vkFormat(VK_FORMAT_UNDEFINED)
      ,_isDepthBuffer(false)
#endif
{
  this->read(filepath, attrNames, channelFilter);
}


Image::Image(Image const & img)
    : width(img.width)
      ,height(img.height)
      ,numComps(img.numComps)
      ,bytesPerComp(img.bytesPerComp)
      ,type(img.type)
      ,data(nullptr)
      ,inChannelMode(img.inChannelMode)
      ,isCubeMap(false)
      ,numMipMaps(1)
      ,inGPU(false)
#if USING_VULKAN
      ,vkFormat(VK_FORMAT_UNDEFINED)
      ,_isDepthBuffer(img._isDepthBuffer)
#endif
{
  this->alloc();
  memcpy(&this->data[0], &img.data[0], img.bytesTotal);
}

Image::~Image() { destroy(); }


void Image::set(u32 w, u32 h, u8 numComps, u8 bytesPerComp, u8 type, bool isDepthBuffer)
{
  this->destroy();
  
  this->width = w;
  this->height = h;
  this->numComps = numComps;
  this->bytesPerComp = bytesPerComp;
  this->type = type;

#if USING_VULKAN
  this->_isDepthBuffer = isDepthBuffer;
#endif

  this->alloc();
}

void Image::crop(u32 startX, u32 startY, u32 endX, u32 endY)
{
  u32 newWidth = endX - startX;
  u32 newHeight = endY - startY;

  if(newWidth > this->width || newHeight > this->height)
  {
    throw std::runtime_error("Image::crop: new dimensions are larger than original");
  }

  u32 newPitch = newWidth * this->bytesPerComp * this->numComps;
  u32 newBytesTotal = newPitch * newHeight;

  u8 * newData = new u8[newBytesTotal];

  u32 oldPitch = this->pitch;
  u32 oldBytesPerComp = this->bytesPerComp;
  u32 oldNumComps = this->numComps;

  u8 * oldData = this->data;

  this->width = newWidth;
  this->height = newHeight;

  this->pitch = newPitch;
  this->bytesPerComp = oldBytesPerComp;
  this->numComps = oldNumComps;
  this->data = newData;

  for(u32 y = 0; y < newHeight; y++)
  {
    memcpy(&this->data[y * newPitch], &oldData[(y + startY) * oldPitch + startX * oldBytesPerComp * oldNumComps], newPitch);
  }

  delete[] oldData;
}

string const rgbaOrder = "RGBA";
string const xyzOrder  = "XYZA";
// string const rgbTest   = "RGBA";

bool sortRgb(Image::Channel const & a, Image::Channel const & b) { return rgbaOrder.find(a.id) < rgbaOrder.find(b.id); }

bool sortXyz(Image::Channel const & a, Image::Channel const & b) { return xyzOrder.find(a.id) < xyzOrder.find(b.id); }


/// preserve: true if there's pixel data to move into each channel
void Image::setToChannelMode(bool v, bool preserve)
{
  if(v)  /// from regular mode to channel mode
  {
    if(this->inChannelMode) return;

    u32 bytesPerChannel = this->width * this->height * this->bytesPerComp;

    // cout << f("bytesPerComp %, numComps %") % (u16)this->bytesPerComp % (u16)this->numComps << endl;

    this->channels.resize(this->numComps);
    for(u8 i = 0; i < this->numComps; i++)
    {
      // cout << f("allocating channel % with % bytes") % (u16)i % bytesPerChannel << endl;
      this->channels[i]._data.resize(bytesPerChannel);
    }

    if(preserve) /// copy data into each channel
    {
      for(u32 y = 0; y < this->height; y++)
      {
        u32 cy = (this->height - 1) - y;

        u32 byteOffsetY  = y * this->pitch;
        u32 cByteOffsetY = cy * this->width * this->bytesPerComp;

        for(u32 x = 0; x < this->width; x++)
        {
          u32 byteOffset  = byteOffsetY + x * this->bytesPerPixel;
          u32 cByteOffset = cByteOffsetY + x * this->bytesPerComp;

          for(auto & c : channels)
          {
            if(byteOffset + this->bytesPerComp > this->bytesTotal) throw runtime_error("Image::setToChannelMode byteOffset + this->bytesPerComp > this->bytesTotal");
            if(cByteOffset + this->bytesPerComp > bytesPerChannel) throw runtime_error("Image::setToChannelMode cByteOffset + this->bytesPerComp > bytesPerChannel");

            memcpy(&(c.data())[cByteOffset], &this->data[byteOffset], this->bytesPerComp);
            byteOffset += this->bytesPerComp;
          }
        }
      }
    }

    destroy();

    this->inChannelMode = true;
    return;
  }

  /// from channel mode back to regular mode: copy channels to main buffer (interleaved)
  if(!this->inChannelMode) return;

  this->alloc();

  u32 numPixels = this->width * this->height;

  /// make sure the channels are in the right order
  if(rgbaOrder.find(channels[0].id) != string::npos)  /// assume rgb(a)
  {
    sort(channels.begin(), channels.end(), sortRgb);
  }
  else
  {
    sort(channels.begin(), channels.end(), sortXyz);
  }

  for(u32 y = 0; y < height; y++)
  {
    u32 cy = (height - 1) - y;

    u32 byteOffsetY  = y * this->pitch;
    u32 cByteOffsetY = cy * this->width * this->bytesPerComp;

    for(u32 x = 0; x < width; x++)
    {
      u32 byteOffset  = byteOffsetY + x * this->bytesPerPixel;
      u32 cByteOffset = cByteOffsetY + x * this->bytesPerComp;

      for(auto & c : channels)
      {
        memcpy(&this->data[byteOffset], &(c.data())[cByteOffset], this->bytesPerComp);
        byteOffset += this->bytesPerComp;
      }
    }
  }

  this->channels.clear();

  this->inChannelMode = false;
}

void Image::flipVertical()
{
  vector<u8> tmpScanLine(this->pitch);
  u8 * tmpScanLineLoc = &(tmpScanLine.data())[0];

  u32 halfHeight = static_cast<u32>(this->height * 0.5);

  for(u32 y1 = 0; y1 < halfHeight; y1++)
  {
    u32 byteOffsetY1 = y1 * this->pitch;
    u8 * location1   = &this->data[byteOffsetY1];
    memcpy(tmpScanLineLoc, location1, this->pitch);

    u32 y2           = (this->height - 1) - y1;
    u32 byteOffsetY2 = y2 * this->pitch;
    u8 * location2   = &this->data[byteOffsetY2];
    memcpy(location1, location2, this->pitch);

    memcpy(location2, tmpScanLineLoc, this->pitch);
  }
}

/// TODO: this isn't required anymore, but if used in the future will need to be updated
void Image::swapRedBlue()
{
  if(this->numComps < 3) return;

  /// swap red and blue
  u32 numPixels = width * height;
  for(u32 i = 0; i < numPixels; i++)
  {
    /// get the memory locations
    u32 elementLoc = i * this->bytesPerPixel;
    void * redLoc  = &data[elementLoc];
    void * blueLoc = &data[elementLoc + 2 * bytesPerComp];

    /// copy red into a temporary block of memory
    u8 * tmpRed = new u8[bytesPerComp];

    memcpy(tmpRed, redLoc, bytesPerComp);

    /// copy blue into the red memory location
    memcpy(redLoc, blueLoc, bytesPerComp);

    /// copy the temp red into the blue memory location
    memcpy(blueLoc, tmpRed, bytesPerComp);

    delete[] tmpRed;
  }
}

void Image::addAlpha()
{
  if(this->numComps != 3) throw runtime_error("Image::addAlpha: numComps != 3");

  this->convert(4, this->bytesPerComp, this->type);

  this->setChannel(3, 1.f);
}

/// NOTE: called setChannel istead of setComponent because it sets the component for all pixels in the Image (what we are calling a channel)
void Image::setChannel(u8 index, f32 value)
{
  if(index >= this->numComps)  throw runtime_error("Image::setChannel: index >= this->numComps");

  if(this->type == Image::UINT)
    this->_setChannel<u8>(index, value);
  else if(this->type == Image::FP)
  {
    if(this->bytesPerComp == 2)
      this->_setChannel<f16>(index, value);
    else
      this->_setChannel<f32>(index, value);
  }
}

f32x4 Image::getPixel(u32 x, u32 y) const
{
  if(this->type == Image::UINT)
  {
    if(this->bytesPerComp == 1)
      return this->_getPixel<u8>(x, y);
    else if(this->bytesPerComp == 2)
      return this->_getPixel<u16>(x, y);
    else if(this->bytesPerComp == 4)
      return this->_getPixel<u32>(x, y);
  }
  else if(this->type == Image::FP)
  {
    if(this->bytesPerComp == 2)
      return this->_getPixel<f16>(x, y);
    else
      return this->_getPixel<f32>(x, y);
  }
  return f32x4();
}

f32 Image::getAlpha(u32 x, u32 y) const
{
  if(this->type == Image::UINT)
    return this->_getGetAlpha<u8>(x, y);
  else if(this->type == Image::FP)
  {
    if(this->bytesPerComp == 2)
      return this->_getGetAlpha<f16>(x, y);
    else
      return this->_getGetAlpha<f32>(x, y);
  }
  return 0.f;
}

void Image::setPixel(u32 const x, u32 const y, f32x4 const & pixel)
{
  if(this->type == Image::UINT)
    this->_setPixel<u8>(x, y, pixel);
  else if(this->type == Image::FP)
  {
    if(this->bytesPerComp == 2)
      this->_setPixel<f16>(x, y, pixel);
    else
      this->_setPixel<f32>(x, y, pixel);
  }
}

void Image::overlayPixel(u32 const x, u32 const y, f32x4 const & pixel)
{
  if(x >= this->width) return;
  if(y >= this->height) return;

  f32x4 oldPixel = this->getPixel(x, y);
  f32x4 newPixel = f32x4(lerp(oldPixel.xyz(), pixel.xyz(), pixel.w), oldPixel.w);
  this->setPixel(x, y, newPixel);
}


void Image::overlay(f32 const r, f32 const g, f32 const b, Image const & aImg, u32 startX, u32 startY)
{
  f32x3 pixel2(r, g, b);

  for(u32 y2 = 0; y2 < aImg.height; y2++)
  {
    u32 y1 = startY + y2;
    if(y1 >= (this->height - 1)) return;

    u32 byteOffset1 = y1 * this->pitch + startX * this->bytesPerPixel;
    u32 byteOffset2 = y2 * aImg.pitch;

    for(u32 x2 = 0; x2 < aImg.width; x2++)
    {
      u32 x1 = startX + x2;
      if(x1 >= (this->width - 1)) break;

      f32 a = aImg.getAlpha(x2, y2);

      f32x4 pixel1 = this->getPixel(x1, y1);

      f32x3 newPix = lerp(pixel1.xyz(), pixel2, a);
      f32x4 newPixel(newPix, 1);
      this->setPixel(x1, y1, newPixel);
    }
  }
}

void Image::overlay(Image const & oImg, u32 startX, u32 startY)
{
  for(u32 y2 = 0; y2 < oImg.height; y2++)
  {
    u32 y1 = startY + y2;
    if(y1 >= (this->height - 1)) return;

    u32 byteOffset1 = y1 * this->pitch + startX * this->bytesPerPixel;
    u32 byteOffset2 = y2 * oImg.pitch;

    for(u32 x2 = 0; x2 < oImg.width; x2++)
    {
      u32 x1 = startX + x2;
      if(x1 >= (this->width - 1)) break;

      f32x4 newPixel = oImg.getPixel(x2, y2);

      this->setPixel(x1, y1, newPixel);
    }
  }
}


/// 1 refers to this image, 2 refers to aImg
void Image::overlay(f32x3 const & color, Image const & aImg, u32 const xPos1, u32 const yPos1, u32 const xPos2, u32 const yPos2, u32 width, u32 height)
{
  for(u32 y = 0; y < height; y++)
  {
    u32 y1 = yPos1 + y;
    if(y1 >= (this->height - 1)) return;

    u32 y2 = yPos2 + y;

    // u32 byteOffset1 = y1 * this->pitch + xPos1 * this->bytesPerPixel;
    // u32 byteOffset2 = y2 * aImg.pitch + xPos2 * aImg.bytesPerPixel;

    for(u32 x = 0; x < width; x++)
    {
      u32 x1 = xPos1 + x;
      if(x1 >= (this->width - 1)) break;

      u32 x2 = xPos2 + x;

      f32 a = aImg.getAlpha(x2, y2);

      f32x4 pixel1 = this->getPixel(x1, y1);

      f32x3 newPix = lerp(pixel1.xyz(), color, a);
      f32x4 newPixel(newPix, 1);
      this->setPixel(x1, y1, newPixel);
    }
  }
}

// /// 1 refers to this image, 2 refers to aImg
// void Image::overlay2(f32x3 const & color, Image const & aImg, u32 const xPos1, u32 const yPos1, u32 const xPos2, u32 const yPos2, u32 width, u32 height)
// {
//   for(u32 y = 0; y < height; y++)
//   {
//     u32 y1 = yPos1 + y;
//     if(y1 >= (this->height - 1)) return;

//     u32 y2 = yPos2 + y;

//     for(u32 x = 0; x < width; x++)
//     {
//       u32 x1 = xPos1 + x;
//       if(x1 >= (this->width - 1)) break;

//       u32 x2 = xPos2 + x;

//       f32 a = aImg.getAlpha(x2, y2);

//       f32x4 pixel1 = this->getPixel(x1, y1);

//       f32x3 newPix = lerp(pixel1.xyz(), color, a);
//       f32x4 newPixel(newPix, 1);
//       this->setPixel(x1, y1, newPixel);
//     }
//   }
// }


/// @brief 
/// @param color 
/// @param box
void Image::overlayBox(f32x4 const color, f32x4 box)
{
  u32 const startX = box.x;
  u32 const startY = box.y;
  u32 const endX   = box.z;
  u32 const endY   = box.w;

  for(u32 y = startY; y < endY; y++)
  {
    if(y >= (this->height - 1)) return;

    for(u32 x = startX; x < endX; x++)
    {
      if(x >= (this->width - 1)) break;

      f32x4 pixel = this->getPixel(x, y);

      f32x3 newPix = lerp(pixel.xyz(), color.xyz(), color.w);
      f32x4 newPixel(newPix, 1.f);
      this->setPixel(x, y, newPixel);
    }
  }
}

vector<u8> Image::getRawPixel(f32 const r, f32 const g, f32 const b, f32 const a)  // private
{
  vector<u8> rawPixel(this->bytesPerPixel);

  if(this->type == Image::UINT)
    _getRawPixel<u8>(r, g, b, a, rawPixel);
  else if(this->type == Image::FP)
  {
    if(this->bytesPerComp == 2)
      _getRawPixel<f16>(r, g, b, a, rawPixel);
    else
      _getRawPixel<f32>(r, g, b, a, rawPixel);
  }

  return rawPixel;
}

void Image::setToColor(f32 const r, f32 const g, f32 const b, f32 const a)
{
  if(!this->data) /// if in the gpu
  {
    // vkMapMemory
  }
  vector<u8> pixel = this->getRawPixel(r, g, b, a);

  u32 byteOffset = 0;
  for(u32 y = 0; y < height; y++)
  {
    u32 byteOffset = y * this->pitch;
    for(u32 x = 0; x < width; x++)
    {
      memcpy(&data[byteOffset], &pixel[0], this->bytesPerPixel);
      byteOffset += this->bytesPerPixel;
    }
  }
}

void Image::convert(u8 numComps, u8 bytesPerComp, u8 type)
{
  if(this->numComps == numComps && this->bytesPerComp == bytesPerComp && this->type == type)
  {
    // l.d("Image::convert: nothing to do");
    return;
  };

  /// TODO:  channels with ids X Y Z will be in -1 to 1 space, so _convert would need a parameter to specify that, right now only handles 0 - 1
  if(this->inChannelMode) this->setToChannelMode(false);

  u8 oldNumComps     = this->numComps;
  u8 oldBytesPerComp = this->bytesPerComp;
  u8 oldType         = this->type;
  u32 oldPitch       = this->pitch;
  u8 * oldData       = this->data;

  this->numComps     = numComps;
  this->bytesPerComp = bytesPerComp;
  this->type         = type;

  this->alloc();

  _converted = false;

  if(oldType == Image::UINT)
  {
    if(oldBytesPerComp == 1)
    {
      if(this->type == Image::FP)
      {
        if(this->bytesPerComp == 2)
          _convert<u8, f16>(oldData, oldNumComps, oldBytesPerComp, oldPitch);
        else if(this->bytesPerComp == 4)
          _convert<u8, f32>(oldData, oldNumComps, oldBytesPerComp, oldPitch);
      }
      else if(this->type == Image::UINT)
      {
        if(this->bytesPerComp == 1) _convert<u8, u8>(oldData, oldNumComps, oldBytesPerComp, oldPitch);
      }
    }
    if(oldBytesPerComp == 2)
    {
      if(this->type == Image::FP)
      {
        if(this->bytesPerComp == 2)
          _convert<u16, f16>(oldData, oldNumComps, oldBytesPerComp, oldPitch);
        else if(this->bytesPerComp == 4)
          _convert<u16, f32>(oldData, oldNumComps, oldBytesPerComp, oldPitch);
      }
      else if(this->type == Image::UINT)
      {
        if(this->bytesPerComp == 1) _convert<u16, u8>(oldData, oldNumComps, oldBytesPerComp, oldPitch);
      }
    }
  }
  else if(oldType == Image::FP)
  {
    if(oldBytesPerComp == 2)
    {
      if(this->type == Image::UINT)
      {
        if(this->bytesPerComp == 1) _convert<f16, u8>(oldData, oldNumComps, oldBytesPerComp, oldPitch);
      }
      else if(this->type == Image::FP)
      {
        if(this->bytesPerComp == 4)
          _convert<f16, f32>(oldData, oldNumComps, oldBytesPerComp, oldPitch);
        else if(this->bytesPerComp == 2)
          _convert<f16, f16>(oldData, oldNumComps, oldBytesPerComp, oldPitch);
      }
    }
    else if(oldBytesPerComp == 4)
    {
      if(this->type == Image::UINT)
      {
        if(this->bytesPerComp == 1) _convert<f32, u8>(oldData, oldNumComps, oldBytesPerComp, oldPitch);
      }
      else if(this->type == Image::FP)
      {
        if(this->bytesPerComp == 2)
          _convert<f32, f16>(oldData, oldNumComps, oldBytesPerComp, oldPitch);
        else if(this->bytesPerComp == 4)
          _convert<f32, f32>(oldData, oldNumComps, oldBytesPerComp, oldPitch);
      }
    }
  }

  delete[] oldData;

  if(!_converted) throw runtime_error("Image::convert: unsupported conversion");
}

void Image::setSize(u32 w, u32 h, bool preserve, u8 algorithm)
{
  if(preserve)
  {
    if(this->inGPU) /// TODO: 
    {

    }
    else /// CPU
    {
      Image oldImage(*this);

      this->destroy();
      this->width  = w;
      this->height = h;
      this->alloc();

      f32x2 scale((f32)oldImage.width / (f32)this->width, (f32)oldImage.height / (f32)this->height); /// scale to map new to old: > 1 means reducing size

      /// nearest pixel algorithm
      if(algorithm == Image::NEAREST)
      {
        for(u32 y = 0; y < this->height; y++)
        {
          u32 sY = round((f32)y * scale.y);

          u32 byteOffset = y * this->pitch;

          u32 oldRowByteOffset = sY * oldImage.pitch;
          
          for(u32 x = 0; x < this->width; x++)
          {
            // if((byteOffset + this->bytesPerPixel) > this->bytesTotal )
            // {
            //   throw runtime_error(f("Image::setSize: byteOffset + this->bytesPerPixel: % > bytestTotal: %") % (byteOffset + this->bytesPerPixel) % this->bytesTotal);
            // }

            u32 sX = round((f32)x * scale.x);
            u32 oldByteOffset = oldRowByteOffset + sX * this->bytesPerPixel;

            u32 oldBytesOffsetCheck = oldByteOffset + this->bytesPerPixel;

            if(oldBytesOffsetCheck > oldImage.bytesTotal) oldByteOffset = oldImage.bytesTotal - this->bytesPerPixel;
            // if(oldBytesOffsetCheck > oldImage.bytesTotal )
            // {
            //   throw runtime_error(f("Image::setSize: oldByteOffset + this->bytesPerPixel: % > bytestTotal: %") % oldBytesOffsetCheck % oldImage.bytesTotal);
            // }

            memcpy(&this->data[byteOffset], &oldImage.data[(u32)oldByteOffset], this->bytesPerPixel);
            
            byteOffset += this->bytesPerPixel;

            // u32 oldX = round((f32)x * scale.x);
            // oldByteOffset += oldX * this->bytesPerPixel;
          }
        }
      }
      else if(algorithm == Image::BILINEAR)
      {
        for(u32 y = 0; y < this->height; y++)
        {
          f32 sY = static_cast<f32>(y) * scale.y;

          u32 y1 = static_cast<u32>(floor(sY));
          u32 y2 = min(this->height, y1 + 1);

          for(u32 x = 0; x < this->width; x++)
          {
            f32 sX = static_cast<f32>(x) * scale.x;

            u32 x1 = static_cast<u32>(floor(sX));
            u32 x2 = min(this->width, x1 + 1);
            
            f32x4 pixel1 = oldImage.getPixel(x1, y1);
            f32x4 pixel2 = oldImage.getPixel(x1, y2);
            f32x4 pixel3 = oldImage.getPixel(x2, y1);
            f32x4 pixel4 = oldImage.getPixel(x2, y2);

            f32 dX1 = x2 - x1;
            f32 dX2 = sX - x1;

            f32x4 lPixel1 = pixel1 + ((pixel3 - pixel1) / dX1) * dX2;
            f32x4 lPixel2 = pixel2 + ((pixel4 - pixel2) / dX1) * dX2;

            f32x4 finalPixel = lPixel1 + ((lPixel2 - lPixel1) / (y2 - y1)) * (sY - y1);

            this->setPixel(x, y, finalPixel);
          }
        }
      }
      else if(algorithm == Image::BICUBIC)
      {
        u32 kernalSize = 5;
        vector<f32x4> kernal(kernalSize * kernalSize);
        for(u32 y = 0; y < 25; y++)
        {
          for(u32 x = 0; x < 25; x++)
          {
            kernal[y * 5 + x] = f32x4(0.0f);
          }

          // kernal[i] = f32x4(0.0f);
        }

        for(u32 y = 0; y < this->height; y++)
        {
          f32 sY = static_cast<f32>(y) * scale.y;

          for(u32 x = 0; x < this->width; x++)
          {
            f32 sX = static_cast<f32>(x) * scale.x;
          }
        }
        

      }
    }

    return;
  }

  /// not preserving image data
  destroy();

  this->width  = w;
  this->height = h;

  this->alloc();

#if USING_VULKAN
  if(this->inGPU)
  {
    this->sendToGPU();
  }
#endif  
}

/// iImage: incoming image
void Image::insert(Image const & iImg, u32 posX, u32 posY)
{
  if(this->numComps != iImg.numComps) throw runtime_error("Image::insert: numComps different");
  if(this->bytesPerComp != iImg.bytesPerComp) throw runtime_error("Image::insert: bytesPerComp different");
  if(this->type != iImg.type) throw runtime_error("Image::insert: types are different");

  // u32 byteOffset = 0;
  for(u32 y = 0; y < iImg.height; y++)
  {
    u32 byteOffset = (posY + y) * this->pitch + posX * this->bytesPerPixel;
    u32 iByteOffset = y * iImg.pitch;

    for(u32 x = 0; x < iImg.width; x++)
    {
      if(byteOffset + iImg.bytesPerPixel > this->bytesTotal) throw runtime_error("Image::insert: outside memory range");
      if(iByteOffset + iImg.bytesPerPixel > iImg.bytesTotal)  throw runtime_error("Image::insert: outside memory range");

      memcpy(&this->data[byteOffset], &iImg.data[iByteOffset], iImg.bytesPerPixel);
      byteOffset += this->bytesPerPixel;
      iByteOffset += iImg.bytesPerPixel;
    }
  }
}

#if USING_VULKAN

void Image::sendToGPU()
{
  if(!this->data)
  {
    throw runtime_error("Image::setToGpu: no data to send to gpu");
  }

  if(this->numComps == 3)  /// pad with a alpha channel
  {
    // l.d("Image::sendToGPU: currently 3 components, adding alpha");
    this->addAlpha();
  }

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  VkImageUsageFlags vkUsage;
  if(_isDepthBuffer) { vkUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; }
  else
  {
    vkUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

    VkDeviceSize imageSize = bytesTotal;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingBufferMemory);

    if(this->data)
    {
      void * sData;
      vkMapMemory(global::vkDevice, stagingBufferMemory, 0, imageSize, 0, &sData);
      memcpy(sData, this->data, static_cast<size_t>(imageSize));
      vkUnmapMemory(global::vkDevice, stagingBufferMemory);
    }
  }

  delete[] this->data;
  this->data = nullptr;

  /// create image memory
  VkImageCreateInfo imageInfo{};
  imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType     = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width  = static_cast<uint32_t>(this->width);
  imageInfo.extent.height = static_cast<uint32_t>(this->height);
  imageInfo.extent.depth  = 1;
  imageInfo.mipLevels     = 1;
  imageInfo.arrayLayers   = 1;
  imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;    // VK_IMAGE_TILING_LINEAR
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;  // VK_IMAGE_LAYOUT_PREINITIALIZED
  imageInfo.usage         = vkUsage;
  imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.flags         = 0;  // Optional

  this->vkFormat = VK_FORMAT_UNDEFINED;

  if(_isDepthBuffer)
  {
    if(this->bytesPerComp == 2) { this->vkFormat = VK_FORMAT_D16_UNORM; }
    if(this->bytesPerComp == 3)
    {
      this->vkFormat = VK_FORMAT_D24_UNORM_S8_UINT;  /// no 24bit format without stencil buffer
    }
    else if(this->bytesPerComp == 4)
    {
      this->vkFormat = VK_FORMAT_D32_SFLOAT;
    }
  }
  else
  {
    if(this->type == Image::UINT)
    {
      if(this->bytesPerComp == 1)
      {
        if     (this->numComps == 1) this->vkFormat = VK_FORMAT_R8_UNORM;
        else if(this->numComps == 3) this->vkFormat = VK_FORMAT_R8G8B8_SRGB;
        else if(this->numComps == 4) this->vkFormat = VK_FORMAT_R8G8B8A8_SRGB;
      }
    }
    else /// assume floating point
    {
      if(this->bytesPerComp == 2) /// half, 16 floating point
      {
        if     (this->numComps == 1) this->vkFormat = VK_FORMAT_R16_SFLOAT;
        else if(this->numComps == 3) this->vkFormat = VK_FORMAT_R16G16B16_SFLOAT;
        else if(this->numComps == 4) this->vkFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
      }
      else /// assume 4: 32bit floating point
      {
        if     (this->numComps == 1) this->vkFormat = VK_FORMAT_R32_SFLOAT;
        else if(this->numComps == 3) this->vkFormat = VK_FORMAT_R32G32B32_SFLOAT;
        else if(this->numComps == 4) this->vkFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
      }
    }
  }

  if(this->vkFormat == VK_FORMAT_UNDEFINED) { throw runtime_error("unsupported image format"); }

  imageInfo.format = this->vkFormat;

  if(vkCreateImage(global::vkDevice, &imageInfo, nullptr, &vkImage) != VK_SUCCESS) { throw runtime_error("failed to create image"); }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(global::vkDevice, vkImage, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize  = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if(vkAllocateMemory(global::vkDevice, &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS)
  {
    throw runtime_error("failed to allocate image memory");
  }

  vkBindImageMemory(global::vkDevice, vkImage, textureImageMemory, 0);

  if(_isDepthBuffer) { vkImageView = createImageView(vkImage, imageInfo.format, VK_IMAGE_ASPECT_DEPTH_BIT); }
  else
  {
    transitionImageLayout(vkImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    /// copy buffer to image memory
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset                    = 0;
    region.bufferRowLength                 = 0;
    region.bufferImageHeight               = 0;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;
    region.imageOffset                     = {0, 0, 0};
    region.imageExtent                     = {this->width, this->height, 1};

    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    endSingleTimeCommands(commandBuffer);

    vkDestroyBuffer(global::vkDevice, stagingBuffer, nullptr);
    vkFreeMemory(global::vkDevice, stagingBufferMemory, nullptr);

    /// to prepar it to be sampled in the shaders
    transitionImageLayout(vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkImageView = createImageView(vkImage, imageInfo.format, VK_IMAGE_ASPECT_COLOR_BIT);
  }

  this->inGPU = true;
}

/// static methods
VkImageView Image::createImageView(VkImage const image, VkFormat const format, VkImageAspectFlags const aspectFlags)
{
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image                           = image;
  viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format                          = format;
  viewInfo.subresourceRange.aspectMask     = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = 1;
  // viewInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
  // viewInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
  // viewInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
  // viewInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;

  VkImageView imageView;
  if(vkCreateImageView(global::vkDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create texture image view");
  }

  return imageView;
}

#endif

/// private

void Image::tally()
{
  this->bytesPerPixel = this->numComps * this->bytesPerComp;
  
  u32 bitsPerPixel = this->bytesPerPixel * 8;
  this->pitch      = ((((bitsPerPixel * width) + 31) / 32) * 4);  // "In FreeImage each scanline starts at a 32-bit boundary for performance reasons."

  // u32 roughPitch   = this->width * this->bytesPerPixel;
  // if(this->pitch != roughPitch) throw runtime_error("tally: pitch calculations differ");  /// for debugging

  this->bytesTotal    = this->height * this->pitch;
  // this->bytesTotal    = this->width * this->height * this->bytesPerPixel;

  switch(this->type)
  {
    case Image::INT:
    {
      typeStr = "INT";
      break;
    }
    case Image::UINT:
    {
      typeStr = "UINT";
      break;
    }
    case Image::FP:
    {
      typeStr = "FP";
      break;
    }
    default:
    {
      typeStr = "UNDEFINED";
      break;
    }
  }
}

void Image::alloc(bool cpuAlloc)
{
  this->tally();

  if(cpuAlloc) { this->data = new u8[this->bytesTotal]; }

  // l.d(f("Image::alloc: w: %, h: %, numComps: %, bytesPerComp: %, type: %, total bytes: %") % this->width % this->height % (u16)this->numComps %
  //     (u16)this->bytesPerComp % (u16)this->type % this->bytesTotal);
}

void Image::destroy()
{
  if(this->inChannelMode)
  {
    this->channels.clear();
    return;
  }
  if(this->data)  /// in cpu
  {
    delete[] this->data;
    this->data = nullptr;
  }
  else  /// in gpu
  {
#if USING_VULKAN
    vkDestroyImageView(global::vkDevice, vkImageView, nullptr);
    vkDestroyImage(global::vkDevice, vkImage, nullptr);
    vkFreeMemory(global::vkDevice, textureImageMemory, nullptr);
#endif
  }
}

/// static
#if USING_VULKAN
void Image::transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier{};
  barrier.sType     = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;

  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  barrier.image                           = image;
  barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel   = 0;
  barrier.subresourceRange.levelCount     = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else
  {
    throw invalid_argument("unsupported layout transition");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

  endSingleTimeCommands(commandBuffer);
}
#endif


Imf::PixelType _getImfPixelType(Image const * img)
{
  if(img->type == Image::UINT) { return Imf::UINT; }
  else
  {
    if(img->bytesPerComp == 2) return Imf::HALF;
    return Imf::FLOAT;
  }
}    

struct my_error_mgr {
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};
typedef struct my_error_mgr * my_error_ptr;
METHODDEF(void) my_error_exit(j_common_ptr cinfo)
{
  my_error_ptr myerr = (my_error_ptr) cinfo->err;
  (*cinfo->err->output_message) (cinfo);
  longjmp(myerr->setjmp_buffer, 1);
}


void Image::read(string const & filepath, vector<string> attrNames, vector<string> channelFilter)
{
  if(!filesystem::exists(filepath))
  {
    throw runtime_error(f("% doesn't exist") % filepath);
  }

  this->filepath = filepath;

  l.d(f("reading %") % this->filepath);

  string ext = filesystem::path(filepath).extension().string();
  // string ext = boost::filesystem::path(filepath).extension().string();
  if(ext == ".exr")
  {
    Imf::InputFile inputFile(filepath.c_str());

    Imf::Header header = inputFile.header();
    Imath::Box2i dw = header.dataWindow();
    Imath::Box2i res = header.displayWindow();

    this->width = res.max.x + 1;
    this->height = res.max.y + 1;

    // this->width  = dw.max.x - dw.min.x + 1;
    // this->height = dw.max.y - dw.min.y + 1;
    // l.i(f("width %, height %") % width % height);

    for(auto const & attrName: attrNames)
    {
      Imf::Header::Iterator hItr = header.find(attrName.c_str());
      if(hItr == header.end())
      {
        // l.w(f("couldn't find % in %") % attrName % filepath);
        continue;
      }
      Imf::Attribute & attr = hItr.attribute();
      string typeName = attr.typeName();
      if(typeName == "float")
      {
        Imf::FloatAttribute sAttr = Imf::TypedAttribute<float>::cast(attr); 
        metaAttrs[attrName] = to_string(sAttr.value());
      }
      else if(typeName == "string")
      {
        Imf::StringAttribute sAttr = Imf::TypedAttribute<string>::cast(attr); 
        metaAttrs[attrName] = sAttr.value();
      }
      // else if(typeName == "") TODO: perhaps there's others of interest
    }

    // i32 offset = -dw.min.x - dw.min.y * this->width;  /// TODO: is this necessary?
    i32 offset = 0;

    Imf::ChannelList const & allChannels = header.channels();

    Imf::ChannelList channels;
    for(auto const & comp : channelFilter) // layer
    {
      Imf::ChannelList::ConstIterator cIter = allChannels.find(comp);
      if(cIter == allChannels.end())
      {
        throw runtime_error(f("failed to find %") % comp);
        // continue;
      }
      channels.insert(cIter.name(), cIter.channel());
    }

    u32 numChannels = 0;

    for(auto li = channels.begin(); li != channels.end(); ++li)
    {
      switch(li.channel().type)
      {
        case Imf::UINT:
        {
          if(this->type == Image::UNDEFINED)
          {
            this->type         = Image::UINT;
            this->bytesPerComp = 4;
          }
          else  /// make sure all channels are the same
          {
            if(this->type != Image::UINT) throw runtime_error(f("Image::read type mismatch"));
          }
        }
        break;

        case Imf::HALF:
        {
          if(this->type == Image::UNDEFINED)
          {
            this->type         = Image::FP;
            this->bytesPerComp = 2;
          }
          else
          {
            if(this->type != Image::FP) throw runtime_error(f("Image::read type mismatch"));
            if(this->bytesPerComp != 2) throw runtime_error(f("Image::read bytesPerComp mismatch"));
          }
        }
        break;

        case Imf::FLOAT:
        {
          if(this->type == Image::UNDEFINED)
          {
            this->type         = Image::FP;
            this->bytesPerComp = 4;
          }
          else
          {
            if(this->type != Image::FP) throw runtime_error(f("Image::read type mismatch"));
            if(this->bytesPerComp != 4) throw runtime_error(f("Image::read bytesPerComp mismatch"));
          }
        }
        break;
      }
      numChannels++;
    }

    this->numComps = numChannels;
    this->setToChannelMode(true);

    Imf::PixelType imfType = _getImfPixelType(this);

    u32 xStride = this->bytesPerComp;
    u32 yStride = this->bytesPerComp * this->width;

    u32 i = 0;
    Imf::FrameBuffer frameBuffer;
    for(auto li = channels.begin(); li != channels.end(); ++li)
    {
      Image::Channel & c = this->channels[i++];
      string cName(li.name());
      c.id = cName.back();
      frameBuffer.insert(li.name(), Imf::Slice(imfType, reinterpret_cast<i8 *>(c.data() + offset), xStride, yStride, 1, 1, 0.0));
    }
    
    inputFile.setFrameBuffer(frameBuffer);
    inputFile.readPixels(dw.min.y, dw.max.y); /// TODO: check if this should be the full res

    this->setToChannelMode(false);
  }

  else if(ext == ".jpg" || ext == ".jpeg")
  {
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    
    FILE * infile = fopen(filepath.c_str(), "rb");
    if(!infile) throw runtime_error(f("failed to open %") % filepath);

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    
    if(setjmp(jerr.setjmp_buffer)) 
    {
      jpeg_destroy_decompress(&cinfo);
      fclose(infile);
      throw runtime_error(f("Image::read: setjmp failed for %") % filepath);
    }
    
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);
    
    u32 row_stride = cinfo.output_width * cinfo.output_components;

    this->width = cinfo.output_width;
    this->height = cinfo.output_height;
    this->numComps = cinfo.output_components;
    this->bytesPerComp = 1;
    this->type = Image::UINT;
    this->alloc();
    
    JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    u32 byteOffset = (this->height * this->pitch) - this->pitch;
    while(cinfo.output_scanline < cinfo.output_height) 
    {
      jpeg_read_scanlines(&cinfo, buffer, 1);
      memcpy(&this->data[byteOffset], buffer[0], row_stride);
      byteOffset -= this->pitch;
    }

    jpeg_finish_decompress(&cinfo);
    
    jpeg_destroy_decompress(&cinfo);

    fclose(infile);
  }

#ifndef SSE_CENTOS7

  else if(ext == ".tif" || ext == ".tiff")
  {
    TIFF * tifFile = TIFFOpen(filepath.c_str(), "r");

    TIFFRGBAImage tImg;
    char emsg[1024];
    if(!TIFFRGBAImageBegin(&tImg, tifFile, 0, emsg)) 
    {
      throw runtime_error(f("TIFFRGBAImageBegin failed for %") % filepath);
    }

    this->width = tImg.width;
    this->height = tImg.height;
    this->numComps = tImg.samplesperpixel;
    if(this->numComps == 1) this->numComps = 2; /// NOTE: TIFFRGBAImageGet was crashing but I had a couple of beers in me and tried this whacky hail mary and it worked
    
    this->bytesPerComp = 1; /// from the docs: "TIFFRGBAImageGet converts non-8-bit images by scaling sample values"

    if(this->numComps < 3 && tImg.bitspersample > 8)
    {
      this->bytesPerComp = tImg.bitspersample / 8; /// but seems that it will still be 16bits if not rgb or rgba
    }
    
    this->type = Image::UINT;
    
    this->alloc();

    if(!TIFFRGBAImageGet(&tImg, reinterpret_cast<uint32_t *>(this->data), this->width, this->height))
    {
      throw runtime_error(f("TIFFRGBAImageGet failed for %") % filepath);
    }

    TIFFRGBAImageEnd(&tImg);
    TIFFClose(tifFile);
  }

  else if(ext == ".hdr")
  {
    HDRLoaderResult r;
    HDRLoader::load(filepath.c_str(), r);

    this->width = r.width;
    this->height = r.height;
    this->numComps = 3;
    this->bytesPerComp = 4;
    this->type = Image::FP;
    this->alloc();

    memcpy(&this->data[0], &r.cols[0], this->bytesTotal);

    delete [] r.cols;

    this->flipVertical(); /// TODO: improve this
  }

#endif

  else if(ext == ".png")
  {
    FILE * fp = fopen(filepath.c_str(), "rb");
    if(!fp) 
    {
      throw runtime_error(f("fopen failed for %") % filepath);
    }

    spng_ctx * ctx = spng_ctx_new(0);
    if(!ctx) throw runtime_error("spng_ctx_new failed");
  
    spng_set_crc_action(ctx, SPNG_CRC_USE, SPNG_CRC_USE);

    size_t const limit = 1024 * 1024 * 64;
    spng_set_chunk_limits(ctx, limit, limit);
    
    spng_set_png_file(ctx, fp);

    struct spng_ihdr ihdr;
    i32 ret = spng_get_ihdr(ctx, &ihdr);
    if(ret > 0)
    {
      throw runtime_error(f("% failed to load because spng_get_ihdr failed because...\n%") % filepath % spng_strerror(ret));
    }

    this->width = ihdr.width;
    this->height = ihdr.height;
    if(ihdr.color_type == SPNG_COLOR_TYPE_TRUECOLOR_ALPHA) this->numComps = 4;
    else if(ihdr.color_type == SPNG_COLOR_TYPE_TRUECOLOR) this->numComps = 3;
    else if(ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE_ALPHA) this->numComps = 2;
    else if(ihdr.color_type == SPNG_COLOR_TYPE_GRAYSCALE) this->numComps = 1;
    else throw runtime_error(f("% failed to load because of an unsupported color type: %") % filepath % (u16)ihdr.color_type);
    
    if(ihdr.bit_depth == 8) this->bytesPerComp = 1;
    else if(ihdr.bit_depth == 16) this->bytesPerComp = 2;
    else if(ihdr.bit_depth == 32) this->bytesPerComp = 4;
    else throw runtime_error(f("% failed to load because a bit depth of % is unsupported") % filepath % (u16)ihdr.bit_depth);

    this->type = Image::UINT;
    this->alloc();

    size_t image_size;
    ret = spng_decoded_image_size(ctx, SPNG_FMT_PNG, &image_size);
    if(ret > 0) throw runtime_error("spng_decoded_image_size failed");

    ret = spng_decode_image(ctx, this->data, image_size, SPNG_FMT_PNG, 0);
    if(ret) throw runtime_error(f("% failed to load because spng_decode_image failed because...\n%") % filepath % spng_strerror(ret));

    this->flipVertical();

    spng_ctx_free(ctx);
  }

  else if(ext == ".tga" || ext == ".TGA")
  {
    FILE * file = std::fopen(filepath.c_str(), "rb");
    tga::StdioFileInterface stdFile(file);
    tga::Decoder decoder(&stdFile);
    tga::Header header;
    if(!decoder.readHeader(header))
    {
      throw runtime_error(f("tga::Decoder::readHeader failed for %") % filepath);
    }
    
    tga::Image tImg;
    tImg.bytesPerPixel = header.bytesPerPixel();
    tImg.rowstride = header.width * header.bytesPerPixel();

    this->width = header.width;
    this->height = header.height;
    this->numComps = tImg.bytesPerPixel;
    this->bytesPerComp = 1;
    this->type = Image::UINT;
    this->alloc();

    tImg.pixels = &this->data[0];

    if(!decoder.readImage(header, tImg, nullptr))
    {
      throw runtime_error(f("tga::Decoder::readImage failed for %") % filepath);
    }

    // "Optional post-process to fix the alpha channel in some TGA files where alpha=0 for all pixels when it shouldn't"
    decoder.postProcessImage(header, tImg);

    // if(header.topToBottom())
    // {
      this->flipVertical();
    // }
  }
}

void Image::write(string const & filepath)
{
  this->filepath = filepath;

  string ext = filesystem::path(filepath).extension().string();
  // string ext = boost::filesystem::path(filepath).extension().string();

  l.d(f("writing to %") % this->filepath);

  if(ext == ".exr")
  {
    /// TMP
    // if(this->type == Image::UINT) 
    // {
    //   if(this->bytesPerComp == 1) /// convert to f16
    //   {
    //     this->convert(this->numComps, 2, Image::FP);
    //   }
    // }

    Imf::PixelType imfType = _getImfPixelType(this);

    Imf::Header header(this->width, this->height);

    header.compression() = Imf::PIZ_COMPRESSION; // ZIP_COMPRESSION;

    /// write header attributes
    for(auto const & ma: this->metaAttrs)
    {
      Imf::StringAttribute exrStrAttr(ma.second);
      header.insert(ma.first, exrStrAttr);
    }

    string firstChannel("R");
    if(this->numComps == 1) firstChannel = "Y";

    header.channels().insert(firstChannel.c_str(), Imf::Channel(imfType));
    if(this->numComps > 1) header.channels().insert("G", Imf::Channel(imfType));
    if(this->numComps > 2) header.channels().insert("B", Imf::Channel(imfType));
    if(this->numComps > 3) header.channels().insert("A", Imf::Channel(imfType));
    
    Imf::OutputFile outFile(filepath.c_str(), header);

    this->setToChannelMode(true, true);

    u32 xStride = this->bytesPerComp;
    u32 yStride = this->bytesPerComp * this->width;

    Imf::FrameBuffer frameBuffer;
    frameBuffer.insert(firstChannel.c_str(), Imf::Slice(imfType, reinterpret_cast<i8 *>(this->channels[0].data()), xStride, yStride));
    if(this->numComps > 1) frameBuffer.insert("G", Imf::Slice(imfType, reinterpret_cast<i8 *>(this->channels[1].data()), xStride, yStride));
    if(this->numComps > 2) frameBuffer.insert("B", Imf::Slice(imfType, reinterpret_cast<i8 *>(this->channels[2].data()), xStride, yStride));
    if(this->numComps > 3) frameBuffer.insert("A", Imf::Slice(imfType, reinterpret_cast<i8 *>(this->channels[3].data()), xStride, yStride));

    outFile.setFrameBuffer(frameBuffer);
    outFile.writePixels(this->height);

    this->setToChannelMode(false);
  }
  else if(ext == ".jpg" || ext == ".jpeg")
  {
    i32 const quality = 90;

    u8 numComps = min((u8)3, this->numComps);

    this->convert(numComps, 1, Image::UINT);

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    
    cinfo.err = jpeg_std_error(&jerr);

    jpeg_create_compress(&cinfo);

    FILE * outfile = fopen(filepath.c_str(), "wb");
    if(!outfile) throw runtime_error(f("failed to open %") % filepath);

    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = this->width; 
    cinfo.image_height = this->height;
    cinfo.input_components = this->numComps;
    cinfo.in_color_space = JCS_RGB;
    
    jpeg_set_defaults(&cinfo);
    
    jpeg_set_quality(&cinfo, quality, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    i32 row_stride = this->width * this->numComps;

    JSAMPROW row_pointer[1];

    /// reverse order from the example
    while(cinfo.next_scanline < cinfo.image_height) {
      u32 loc = (cinfo.image_height - 1) - cinfo.next_scanline;
      row_pointer[0] = &this->data[loc * row_stride];
      jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    
    fclose(outfile);

    jpeg_destroy_compress(&cinfo);    
  }
  else if(ext == ".hdr")
  {
    
  }
}
