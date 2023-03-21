
#include "gt_base/imgUtils.h"
#include "gt_base/types/f32x2.h"
#include "gt_base/types/f32x3.h"
#include "gt_base/types/f32x4.h"
#include "gt_base/functions.h"
#include "gt_base/string.h"
#include "gt_base/Image.h"
#include "gt_base/Text.h"
#include <OpenColorIO/OpenColorTransforms.h>
#include <filesystem>

using namespace gt;
using namespace std;

namespace OCIO = OCIO_NAMESPACE;
OCIO::ConstConfigRcPtr gt::ocioConfig = nullptr;

void gt::transformColor(Image & img, string const & inColorSpace, string const & outColorSpace)
{
  if(!ocioConfig)
  {
    i8 * ocioVal = getenv("OCIO");
    if(!ocioVal)
    {
      throw runtime_error("imgUtils::transformColor: OCIO env var not set");
    }
    
    if(!filesystem::exists(ocioVal)) throw runtime_error(f("imgUtils::transformColor: % doesn't exist") % ocioVal);

    try
    {
      ocioConfig = OCIO::GetCurrentConfig();
    }
    catch(OCIO::Exception & exception)
    {
      throw runtime_error(f("imgUtils::transformColor: OpenColorIO error caught...\n%") % exception.what());
    }
  }

  // l.r(f("OCIO version %\n") % OCIO::GetVersion());

  try
  {
    OCIO::ConstProcessorRcPtr processor = ocioConfig->getProcessor(inColorSpace.c_str(), outColorSpace.c_str());  

    OCIO::BitDepth ocioBitDepth = OCIO::BitDepth::BIT_DEPTH_UINT8;
    if(img.type == Image::FP)
    {
      if(img.bytesPerComp == 2) ocioBitDepth = OCIO::BitDepth::BIT_DEPTH_F16;
      else ocioBitDepth = OCIO::BitDepth::BIT_DEPTH_F32;
    }

    OCIO::ConstCPUProcessorRcPtr ocioPrc = processor->getOptimizedCPUProcessor(ocioBitDepth, ocioBitDepth, OCIO::OptimizationFlags::OPTIMIZATION_VERY_GOOD);

    ptrdiff_t chanStrideBytes = img.bytesPerComp;
    ptrdiff_t xStrideBytes = img.bytesPerPixel;
    ptrdiff_t yStrideBytes = img.width * img.bytesPerPixel;

    // l.i("OCIO conversion from: %, to: %" % inColorSpace % outColorSpace);

    OCIO::PackedImageDesc ocioImg(img.data, img.width, img.height, img.numComps, ocioBitDepth, chanStrideBytes, xStrideBytes, yStrideBytes);
    ocioPrc->apply(ocioImg);
  }

  catch(OCIO::Exception & exception)
  {
    throw runtime_error(f("imgUtils::transformColor: OpenColorIO error caught...\n%") % exception.what());
  }  
}

/// @brief  NOTE: legacy: use Text::overlay instead
/// @param img 
/// @param text 
/// @param startPos 
/// @param bgColor 
/// @param bgPadding 
void gt::drawText(Image & img, Text const & text, f32x2 const & startPos, f32x4 bgColor, f32 bgPadding)
{
  f32 textStartY = startPos.y + bgPadding;

  f32 const maxWidth = img.width - startPos.x - bgPadding * 2;

  if(bgColor.w > 0) /// draw a box behind the text
  {
    f32 const lineHeight = text.lineHeight + text.margin;

    /// calculate the size of the box
    f32 xPos    = 0.f;
    f32 yPos    = 0.f;
    f32 width = 0.f;

    for(auto const c : text.value)
    {
      if(c == '\n')
      {
        xPos = 0.f;
        yPos += lineHeight;
        continue;
      }
      Glyph const g = text.glyphs.at(c);
      xPos += g.width;

      width = std::max(width, xPos);

      if(xPos >= maxWidth)
      {
        xPos = 0;
        yPos += lineHeight;
      }
    }

    yPos += yPos > 0 ? lineHeight : 0;

    f32 const endX = startPos.x + width + bgPadding * 2.f;
    f32 const endY = startPos.y + yPos + bgPadding * 2.f;

    f32x4 box(startPos.x, startPos.y, endX, endY);

    img.overlayBox(bgColor, box);

    textStartY += yPos ;
  }

  /// drawing from the top of the box down
  u32 const textStartX = startPos.x + bgPadding;
  u32 xPos = textStartX;
  u32 yPos = textStartY;

  for(auto const c : text.value)
  {
    if(c == '\n')
    {
      xPos = textStartX;
      yPos -= text.lineHeight - text.margin;
      continue;
    }

    Glyph const g = text.glyphs.at(c);

    img.overlay(text.color, *(text.glyphImg), xPos, yPos, text.glyphImg->width * g.tsPosX, text.glyphImg->height * g.tsPosY, g.width, g.height);
    xPos += g.width;

    if(xPos >= maxWidth)
    {
      xPos = textStartX;
      yPos -= text.lineHeight - text.margin;
    }
  }
}

