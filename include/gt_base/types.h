
#pragma once

#ifdef ORCA_ARNOLD
  #include <half.hpp>
#else
  #include <Imath/half.h>
#endif

namespace gt
{

typedef char i8;
typedef short int i16;
typedef int i32;
typedef long long int i64;

typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned long long int u64;

#ifdef ORCA_ARNOLD
  typedef half_float::half f16;
#else
  typedef half f16;
#endif

typedef float f32;
typedef double f64;

#define MAX_U8 255
#define MAX_U16 65535
#define MAX_U32 4294967295

} // namespace gt
