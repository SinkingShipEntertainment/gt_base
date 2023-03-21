
#pragma once

#include "gt_base/types/base.h"
#include <math.h>

namespace gt
{
  
class f32x2;
class f32x3;
class f32x4;

#define PI 3.14159265358979323846

template <typename T> T radiansToDegrees(T radian) {return radian * 180.0 / PI;}

template <typename T> T degreesToRadians(T degree) {return degree / 180.0 * PI;}

template <typename T> T decimal(T & v) {return v - static_cast<i32>(v);}

template <typename T> T lerp(T const & low, T const & high, T const & v) {return low + v * (high - low);}
f32x3 lerp(f32x3 const & low, f32x3 const & high, f32 v);
f32x4 lerp(f32x4 const & low, f32x4 const & high, f32 v);

template <typename T> T remapFrom01(T const & low, T const & high, T const & v) {return lerp(low, high, v);}

template <typename T> T remapTo01(T const & low, T const & high, T const & v) {return (v - low) / (high - low);}
f32x2 remapTo01(const f32x2 & low, f32x2 const & high, f32x2 const & v);

template <typename T> T remap(T fromLow, T fromHigh, T toLow, T toHigh, T v)
{
  T r = (v - fromLow) / (fromHigh - fromLow);
  return lerp(toLow, toHigh, r);
}

template <typename T> T min(T const & v, T const & m) {return v < m ? v : m;}
template <typename T> T max(T const & v, T const & m) {return v > m ? v : m;}
template <typename T> T clamp(T const & n, T const & lower, T const & upper) {return max(lower, min(n, upper));}

f32 magnitude(f32x2 const & v);
f32 magnitude(f32x3 const & v);

void convert(u8 const v, f16 & r);
void convert(u8 const v, f32 & r);
void convert(u16 const v, u8 & r);
void convert(u16 const v, f16 & r);
void convert(u16 const v, f32 & r);
void convert(u32 const v, f32 & r);
void convert(f16 const v, u8 & r);
void convert(f16 const v, f32 & r);
void convert(f32 const v, u8 & r);
void convert(f32 const v, f16 & r);
template<typename T> void convert(T const v, T & r) {r = v;} /// NOTE: seems silly but taking advantage of the compile time type determination for use with templating (see _convert in Image.cpp)


} // namespace gt

