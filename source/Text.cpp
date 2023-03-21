
#include "gt_base/Text.h"
#include "gt_base/types/f32x2.h"
#include "gt_base/string.h"
#include "gt_base/functions.h"
#include "gt_base/Image.h"

using namespace gt;
using namespace std;

#if defined(__linux__)
  string fontTemplatePath("/usr/share/fonts/truetype/%.ttf");
#elif defined(_WIN32)
  string fontTemplatePath("C:/Windows/Fonts/%.ttf");
#endif


Text::Text(u16 size, string const & font, f32x3 const & color, string const value) : size(size), font(font), color(color), value(value), glyphImg(nullptr), margin(0)
{
  // l.r(f("text: pos: %, color: %, font: %, size %\n") % textPos % textColor % font % textSize);

  if(FT_Init_FreeType(&library) != 0) { throw runtime_error("failed to initialize FreeType2 library"); }

  string fontPath(f(fontTemplatePath) % font);
  if(FT_New_Face(library, fontPath.c_str(), 0, &face) != 0)
  {
    FT_Done_FreeType(library);
    throw runtime_error(f("failed to load %") % fontPath);
  }

  if(!(face->face_flags & FT_FACE_FLAG_SCALABLE) || !(face->face_flags & FT_FACE_FLAG_HORIZONTAL))
  {
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    throw runtime_error(f("% not a true type, scalable font") % fontPath);
  }

  FT_Set_Pixel_Sizes(face, this->size, 0);
}


void Text::overlay(Image & img, f32x2 const & pos, bool yBottom, f32x4 bgColor, f32 bgPadding)
{
  u32 ix = 0;

  u32 width = 0;
  bool firstChar = true;

  i32 maxAscent  = 0;
  i32 maxDescent = 0;

  u32 numLines = 0;

  for(u8 const c : this->value)
  {
    u32 charIndex = FT_Get_Char_Index(face, static_cast<u32>(c));

    FT_Load_Glyph(face, charIndex, FT_LOAD_DEFAULT);
    FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

    if(firstChar)
    {
      firstChar = false;
      numLines++;
    }

    if(c == '\n')
    {
      ix = 0;
      numLines++;
      continue;
    }

    ix += (face->glyph->metrics.horiAdvance >> 6) + margin;
    width = std::max(width, ix);

    maxAscent  = gt::max(static_cast<i32>(face->glyph->bitmap_top), maxAscent);
    maxDescent = gt::max(static_cast<i32>(face->glyph->bitmap.rows - face->glyph->bitmap_top), maxDescent);
  }

  u32 lineHeight = maxAscent + maxDescent + margin;

  u32 height = lineHeight * numLines;

  if(bgColor.w > 0.f)
  {
    f32 startY = pos.y;
    f32 const endX = pos.x + width + bgPadding * 2.f;
    f32 endY = pos.y;
    if(yBottom)
    {
      endY += height + bgPadding * 2;
    }
    else
    {
      startY -= height + bgPadding * 2;
    }

    f32x4 box(pos.x, startY, endX, endY);

    img.overlayBox(bgColor, box);
  }
  
  firstChar = true;

  u32 startX = pos.x + bgPadding;
  i32 startY = pos.y;
  if(yBottom)
  {
    startY += height + bgPadding;// * 2;
  }
  else
  {
    startY -= bgPadding;
  }

  ix = startX;  

  for(u8 const c : this->value)
  {
    u32 charIndex = FT_Get_Char_Index(face, static_cast<u32>(c));

    FT_Load_Glyph(face, charIndex, FT_LOAD_DEFAULT);
    FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

    if(firstChar)
    {
      startY -= lineHeight;
      firstChar = false;
    }

    if(c == '\n')
    {
      ix = startX;
      startY -= lineHeight;
      continue;
    }

    u32 iy = startY + face->glyph->bitmap_top;

    for(u32 y = 0; y < face->glyph->bitmap.rows; ++y)
    {
      for(u32 x = 0; x < face->glyph->bitmap.width; ++x)
      {
        u8 val8bit = face->glyph->bitmap.buffer[x + y * face->glyph->bitmap.pitch];

        f32 val32bit;
        convert(val8bit, val32bit);

        f32x4 pix(this->color, val32bit);
        img.overlayPixel(ix + x, iy - y, pix);
      }
    }
    ix += (face->glyph->metrics.horiAdvance >> 6) + margin;
  }
}

/// @brief  caches all characters to an internal texture
void Text::cache()
{
  string const chars(
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "1234567890~!@#$%^&*()_-=+;:"
      "'\",./?[]|\\ <>`\xFF");

  u16 const tImgWidth = 256; /// TODO: dynamically determine what this should be based on font size

  /// NOTE: face->ascender and face->descender aren't reliable
  i32 maxAscent  = 0;
  i32 maxDescent = 0;

  u32 numLines    = 1;
  u16 spaceOnLine = tImgWidth - margin;

  for(u8 const c : chars)
  {
    u32 charIndex = FT_Get_Char_Index(face, static_cast<u32>(c));

    /// render the current glyph
    FT_Load_Glyph(face, charIndex, FT_LOAD_DEFAULT);
    FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

    u32 advance = (face->glyph->metrics.horiAdvance >> 6) + margin;

    /// if the line is full go to the next line
    if(advance > spaceOnLine)
    {
      spaceOnLine = tImgWidth - margin;
      numLines++;
    }

    spaceOnLine -= advance;

    maxAscent  = gt::max(static_cast<i32>(face->glyph->bitmap_top), maxAscent);
    maxDescent = gt::max(static_cast<i32>(face->glyph->bitmap.rows - face->glyph->bitmap_top), maxDescent);
  }

  u32 neededImageHeight = (maxAscent + maxDescent + margin) * numLines + margin;

  /// get the first power of two in which it fits
  u32 tImgHeight = 16;
  while(tImgHeight < neededImageHeight)
  {
    tImgHeight *= 2;
  }

  f32 wScale = 1.0f / tImgWidth;
  f32 hScale = 1.0f / tImgHeight;

  lineHeight   = maxAscent + maxDescent;   /// pixel space line height
  u32 tsLineHeight = lineHeight * hScale;  /// texture / uv space line height

  u32 x = margin;
  u32 y = margin + maxAscent;

  glyphImg = new Image(tImgWidth, tImgHeight, 1, 1);
  glyphImg->setToColor(0);

  /// write all characters to an image file
  for(u8 const c : chars)
  {
    u32 charIndex = FT_Get_Char_Index(face, static_cast<u32>(c));

    /// Render the glyph
    FT_Load_Glyph(face, charIndex, FT_LOAD_DEFAULT);
    FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

    /// see whether the character fits on the current line
    u32 advance = (face->glyph->metrics.horiAdvance >> 6) + margin;
    if(advance > tImgWidth - x)
    {
      x = margin;
      y += (maxAscent + maxDescent + margin);
    }

    /// copy the image from FreeType to the texture at the correct position
    for(u32 row = 0; row < face->glyph->bitmap.rows; ++row)
    {
      for(u32 pixel = 0; pixel < face->glyph->bitmap.width; ++pixel)
      {
        u32 loc = (x + face->glyph->bitmap_left + pixel) + (y - face->glyph->bitmap_top + row) * tImgWidth;

        u8 value = face->glyph->bitmap.buffer[pixel + row * face->glyph->bitmap.pitch];

        memcpy(&glyphImg->data[loc], &value, 1);
      }
    }

    /// recored glyph info
    u32 width  = advance - margin;  /// pixel space glyph width
    u32 height = lineHeight;    // maxAscent + maxDescent + margin;
    f32 normX  = x * wScale;
    f32 normY  = (y - maxAscent) * hScale;
    glyphs[c]  = Glyph(width, height, width * wScale, normX, normY);

    x += advance;
  }
}


Text::~Text()
{
  delete glyphImg;
  FT_Done_Face(face);
  FT_Done_FreeType(library);
}

