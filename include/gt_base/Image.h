
#pragma once

#include "gt_base/types.h"
#include "gt_base/types/f32x3.h"
#include "gt_base/types/f32x4.h"
#include "gt_base/functions.h"
#if USING_VULKAN
  #include <vulkan/vulkan.h>
#endif
#include <string>
#include <vector>
#include <map>
#include <string.h> /// memcpy

/// terms:
/// component: per pixel: ie R at a given pixel
/// channel: single compenent accross an entire image ie R for all pixels

namespace gt
{

class Image
{
 public:

  enum Type
  {
    INT,
    UINT,
    FP,
    UNDEFINED
  };

  struct Channel
  {
    u8 * data() {return _data.data();}
    i8 id; /// R G B A X Y Z
    std::vector<u8> _data;
  };

  Image();
  Image(u32 w, u32 h, u8 numComps, u8 bytesPerComp, u8 type = Image::UINT, bool isDepthBuffer = false);
  Image(std::string const & filepath, std::vector<std::string> const attrNames = {}, std::vector<std::string> channelFilter = {"R", "G", "B", "A", "Y"});
  Image(Image const & img);
  ~Image();

  void set(u32 w, u32 h, u8 numComps, u8 bytesPerComp, u8 type = Image::UINT, bool isDepthBuffer = false);

  void crop(u32 startX, u32 startY, u32 endX, u32 endY);

  f32x4 getPixel(u32 x, u32 y) const;

  f32 getAlpha(u32 x, u32 y) const;

  void setPixel(u32 const x, u32 const y, f32x4 const & pixel);

  void overlayPixel(u32 const x, u32 const y, f32x4 const & pixel);

  /// @brief use the alpha of aImg to draw r, g, b color over the existing image at startX, startY
  /// @param r 
  /// @param g 
  /// @param b 
  /// @param aImg alpha image
  /// @param startX 
  /// @param startY 
  void overlay(f32 const r, f32 const g, f32 const b, Image const & aImg, u32 const startX, u32 const startY);

  /// @brief 
  /// @param oImg other image
  /// @param startX 
  /// @param startY 
  void overlay(Image const & oImg, u32 const startX, u32 const startY);

  void overlay(f32x3 const & color, Image const & aImg, u32 const xPos1, u32 const yPos1, u32 const xPos2, u32 const yPos2, u32 width2, u32 height2);

  void overlayBox(f32x4 const color, f32x4 box);
  // void overlayBox(f32 const r, f32 const g, f32 const b, f32 const a, u32 startX, u32 startY, u32 endX, u32 endY);

  void setToColor(f32 const r, f32 const g = 0, f32 const b = 0, f32 const a = 1);

  void setToChannelMode(bool v, bool preserve = false);  /// for multi-channel exrs

  // channelFilter: could also be  {"albedo.R", "albedo.G", "albedo.B"} {"N.X", "N.Y", "N.Z"} etc
  // attrNames: meta attributes (from OpenEXR headers etc) 
  void read(std::string const & filepath, std::vector<std::string> const attrNames = {}, std::vector<std::string> channelFilter = {"R", "G", "B", "A", "Y"});
  void write(std::string const & filepath);

  void flipVertical();
  
  void swapRedBlue();

  void addAlpha();

  void setChannel(u8 index, f32 value);

  void convert(u8 numComps, u8 bytesPerComp, u8 type);

  void setSize(u32 w, u32 h, bool preserve = false);

  void insert(Image const & iImg, u32 posX, u32 posY);

#if USING_VULKAN
  void sendToGPU();
  static VkImageView createImageView(VkImage const image, VkFormat const format, VkImageAspectFlags const aspectFlags);
  VkImage vkImage;
  VkImageView vkImageView;
  VkFormat vkFormat;
#endif

  u32 width;
  u32 height;
  u8 numComps;
  u8 bytesPerComp;
  u8 type;
  u32 bytesPerPixel;
  u32 bytesTotal;
  u32 pitch; /// TODO: change to bytesPerRow?
  u8 * data; /// TODO: std::vector<u8>?
  bool inChannelMode;
  bool isCubeMap; /// if cube map, assume horizontal format
  u8 numMipMaps;
  bool inGPU;

  std::string typeStr;

  std::vector<Channel> channels; /// for multi-layer exrs TODO: change to map<string, Channel> at somepoint?
  
  std::string filepath;

  std::map<std::string, std::string> metaAttrs; /// meta attributes (from OpenEXR headers etc) 

  void tally();
  void alloc(bool cpuAlloc = true);

 private:

  // void tally();
  // void alloc(bool cpuAlloc = true);
  void destroy();

  /// convert the f32 pixel to the native raw bits suited to this Image
  std::vector<u8> getRawPixel(f32 const r, f32 const g = 0, f32 const b = 0, f32 const a = 0);

  /// getting
  template<typename T>
  f32 _getComp(u32 const byteOffset, u8 const compIndex) const
  {
    T _v;
    memcpy(&_v, &this->data[byteOffset + compIndex * this->bytesPerComp], this->bytesPerComp);
    f32 v;
    gt::convert(_v, v);
    return v;
  }

  /// returns pixel as a f32x4
  template<typename T>
  f32x4 _getPixel(u32 x, u32 y) const
  {
    u32 byteOffset = y * this->pitch + x * this->bytesPerPixel;

    f32 r = 0, g = 0, b = 0;
    f32 a = 1;

    r = _getComp<T>(byteOffset, 0);
    if(this->numComps > 1) g = _getComp<T>(byteOffset, 1);
    if(this->numComps > 2) b = _getComp<T>(byteOffset, 2);
    if(this->numComps > 3) a = _getComp<T>(byteOffset, 3);

    return f32x4(r, g, b, a);
  }

  template<typename T>
  f32 _getGetAlpha(u32 x, u32 y) const
  {
    u32 byteOffset = y * this->pitch + x * this->bytesPerPixel;

    if(this->numComps == 1) return _getComp<T>(byteOffset, 0);
    else if(this->numComps == 4) return _getComp<T>(byteOffset, 3);

    return 0;
  }
  
  template<typename T>
  void _getRawComp(f32 const v, u8 const compIndex, std::vector<u8> & pixel) const
  {
    T c;
    gt::convert(v, c);
    memcpy(&(pixel.data())[compIndex * this->bytesPerComp], &c, this->bytesPerComp);
  }
  
  template<typename T>
  void _getRawPixel(f32 const r, f32 const g, f32 const b, f32 const a, std::vector<u8> & pixel) const
  {
    _getRawComp<T>(r, 0, pixel);
    if(this->numComps > 1) _getRawComp<T>(g, 1, pixel);
    if(this->numComps > 2) _getRawComp<T>(b, 2, pixel);
    if(this->numComps > 3) _getRawComp<T>(a, 3, pixel);
  }

  /// setting
  template<typename T>
  void _setComp(u32 const byteOffset, u8 const compIndex, f32 const v)
  {
    T _v;
    gt::convert(v, _v);
    memcpy(&this->data[byteOffset + compIndex * this->bytesPerComp], &_v, this->bytesPerComp);
  }

  template<typename T>
  void _setPixel(u32 x, u32 y, f32x4 const & pixel)
  {
    u32 byteOffset = y * this->pitch + x * this->bytesPerPixel;

    _setComp<T>(byteOffset, 0, pixel.x);
    if(this->numComps > 1) _setComp<T>(byteOffset, 1, pixel.y);
    if(this->numComps > 2) _setComp<T>(byteOffset, 2, pixel.z);
    if(this->numComps > 3) _setComp<T>(byteOffset, 3, pixel.w);
  }

  template<typename T_OLD, typename T_NEW>
  void _convert(u8 const * oldData, u8 oldNumComps, u8 oldBytesPerComp, u32 oldPitch)
  {
    u32 oldBytesPerPixel = oldNumComps * oldBytesPerComp;

    u32 numPixels = this->width * this->height;
    u32 bytesPerPixel = this->bytesPerComp * this->numComps;

    u8 numComps = std::min(oldNumComps, this->numComps);

    for(u32 y = 0; y < height; y++)
    {
      u32 oldByteOffsetY = y * oldPitch;
      u32 byteOffsetY = y * this->pitch;
      for(u32 x = 0; x < width; x++)
      {
        u32 oldByteOffset = oldByteOffsetY + x * oldBytesPerPixel;
        u32 byteOffset = byteOffsetY + x * bytesPerPixel;
        for(u8 c = 0; c < numComps; c++)
        {
          T_NEW v;
          
          T_OLD const * op = reinterpret_cast<T_OLD const *>(&oldData[oldByteOffset]);
          gt::convert(*op, v);

          memcpy(&this->data[byteOffset], &v, this->bytesPerComp);

          oldByteOffset += oldBytesPerComp;
          byteOffset += this->bytesPerComp;        
        }
      }
    }
    _converted = true;
  }

  template<typename T>
  void _setChannel(u8 index, f32 value)
  {
    for(u32 y = 0; y < this->height; y++)
    {
      u32 rowByteOffset = y * this->pitch;
      for(u32 x = 0; x < this->width; x++)
      {
        u32 byteOffset = rowByteOffset + x * this->bytesPerPixel;
        this->_setComp<T>(byteOffset, index, value);
      }
    }
  }


#if USING_VULKAN
  static void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout); 
  VkDeviceMemory textureImageMemory;
  bool _isDepthBuffer;
#endif

  bool _converted;

};

}  // namespace gt
