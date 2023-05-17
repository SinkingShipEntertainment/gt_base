
#include "gt_base/types/f32x2.h"
#include "gt_base/types/f32x3.h"
#include "gt_base/types/f32x4.h"
#include "gt_base/functions.h"

using namespace gt;


f32x3 gt::lerp(const f32x3 & low, const f32x3 & high, f32 v)
{
  f32x3 r;
  r.x = lerp(low.x, high.x, v);
  r.y = lerp(low.y, high.y, v);
  r.z = lerp(low.z, high.z, v);
  return r;
}

f32x4 gt::lerp(f32x4 const & low, f32x4 const & high, f32 v)
{
  return f32x4(lerp(low.x, high.x, v), lerp(low.y, high.y, v),
               lerp(low.z, high.z, v), lerp(low.w, high.w, v));
}

f32x2 gt::remapTo01(const f32x2 & low, const f32x2 & high, const f32x2 & v)
{
  f32x2 r;
  r.x = (v.x - low.x) / (high.x - low.x);
  r.y = (v.y - low.y) / (high.y - low.y);
  return r;
}

f32 gt::magnitude(f32x2 const & v)
{
  return sqrt(v.x * v.x + v.y * v.y);
}

f32 gt::magnitude(f32x3 const & v)
{
  return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}


void gt::convert(u8 const v, f16 & r)
{
  if(v >= MAX_U8)
  {
    r = static_cast<f16>(1);
    return;
  }
  if(v <= 0)
  {
    r = static_cast<f16>(0);
    return;
  }
  r = static_cast<f16>(v) / static_cast<f16>(MAX_U8);
}

void gt::convert(u8 const v, f32 & r)
{
  if(v >= MAX_U8)
  {
    r = 1.f;
    return;
  }

  if(v <= 0)
  {
    r = 0.f;
    return;
  }

  r = static_cast<f32>(v) / static_cast<f32>(MAX_U8);
}

void gt::convert(u16 const v, u8 & r)
{
  if(v >= MAX_U16)
  {
    r = MAX_U8;
    return;
  }

  if(v <= 0)
  {
    r = 0;
    return;
  }
  
  f32 r_ = static_cast<f32>(v) / static_cast<f32>(MAX_U16);
  r = static_cast<u8>(r_ * static_cast<f32>(MAX_U8));
}

void gt::convert(u16 const v, f16 & r)
{
  if(v >= MAX_U16)
  {
    r = static_cast<f16>(1);
    return;
  }
  if(v <= 0)
  {
    r = static_cast<f16>(0);
    return;
  }

  r = static_cast<f32>(v) / static_cast<f32>(MAX_U16);
}

void gt::convert(u16 const v, f32 & r)
{
  if(v >= MAX_U16)
  {
    r = 1.f;
    return;
  }

  if(v <= 0)
  {
    r = 0.f;
    return;
  }

  r = static_cast<f32>(v) / static_cast<f32>(MAX_U16);
}

void gt::convert(u32 const v, f32 & r)
{
  if(v >= MAX_U32)
  {
    r = 1.f;
    return;
  }

  if(v <= 0)
  {
    r = 0.f;
    return;
  }

  r = static_cast<f32>(v) / static_cast<f32>(MAX_U32);
}

void gt::convert(f16 const v, u8 & r)
{
  if(v >= 1.0)
  {
    r = MAX_U8;
    return;
  }

  if(v <= 0.0)
  {
    r = 0;
    return;
  }

  r = static_cast<u8>(v * static_cast<f16>(MAX_U8));
}

void gt::convert(f16 const v, f32 & r)
{
  r = static_cast<f32>(v);
}

void gt::convert(f32 const v, u8 & r)
{
  if(v >= 1.0f)
  {
    r = MAX_U8;
    return;
  }

  if(v <= 0.0f)
  {
    r = 0;
    return;
  }

  r = static_cast<u8>(v * static_cast<f32>(MAX_U8));
}

void gt::convert(f32 const v, f16 & r)
{
  r = static_cast<f16>(v);
}
