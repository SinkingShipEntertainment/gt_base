
#pragma once

#include "gt_base/types.h"
#include "gt_base/types/f32x3.h"
#include "gt_base/types/f32x4.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include <map>

namespace gt
{

class f32x2;
// class f32x4;
class Image;

/// @brief for caching to a texture
struct Glyph
{
  Glyph() {}

  Glyph(u32 width, u32 height, f32 tsWidth, f32 tsPosX, f32 tsPosY) : width(width), height(height), tsWidth(tsWidth), tsPosX(tsPosX), tsPosY(tsPosY)
  {
  }

  u32 width, height;
  f32 tsWidth, tsPosX, tsPosY;  /// texture / uv space (0 - 1)
};

class Text
{
 public:
  Text(u16 size, std::string const & font, f32x3 const & color, std::string const value = std::string(""));
  ~Text();

  /// @brief  overlays this text over image
  /// @param img 
  /// @param pos 
  /// @param yBottom when true pos.y is the bottom / end of the text, when false it's the top / start of the text
  /// @param bgColor 
  /// @param bgPadding 
  void overlay(Image & img, f32x2 const & pos, bool yBottom = true, f32x4 const bgColor = f32x4(0, 0, 0, 0), f32 bgPadding = 0);

  void cache();

  u16 size;
  std::string font;
  std::string value;
  f32x3 color;

  u16 lineHeight;
  u16 margin;
  Image * glyphImg;
  std::map<u8, Glyph> glyphs;

  FT_Face face;
  FT_Library library;
};

}  // namespace gt
