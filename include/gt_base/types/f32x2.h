
#pragma once

#include "gt_base/types.h"
#include <iostream>

namespace gt
{
class f32x2
{
 public:
  f32x2(f32 s = 0);
	f32x2(f32 x, f32 y);

  void set(f32 x, f32 y);

  f32 length() const;

  f32 const * memAddr() const { return &x; }
  f32 x, y;
};

f32 dot(const f32x2 & v1, f32x2 const & v2);

f32x2 operator+(f32x2 const & p1, f32x2 const & p2);
f32x2 operator+(f32x2 const & p1, f32 const & p2);
void operator+=(f32x2 & p1, f32x2 const & p2);

f32x2 operator-(f32x2 const & p1, f32x2 const & p2);
void operator-=(f32x2 & p1, f32x2 const & p2);

f32x2 operator*(const f32x2 & p1, const f32 & p2);
f32x2 operator*(const f32 & p1, f32x2 const & p2);
f32x2 operator*(const f32x2 & p1, f32x2 const & p2);
void operator*=(f32x2 & v1, f32 const & v2);

f32x2 operator/(f32x2 const & p1, f32x2 const & p2);
f32x2 operator/(f32x2 const & p1, f32 const & p2);
f32x2 operator/(f32 p1, f32x2 const & p2);

bool operator>(const f32x2 & p1, f32x2 const & p2);
bool operator<(const f32x2 & p1, f32x2 const & p2);
bool operator>(const f32x2 & p1, f32 p2);
bool operator==(const f32x2 & p1, f32 p2);
bool operator==(const f32x2 & p1, f32x2 const & p2);
bool operator!=(const f32x2 & p1, f32x2 const & p2);
bool operator!(f32x2 const & p1);

f32 length(f32x2 const & v1, f32x2 const & v2);

/// io
void write(std::ostream & os, f32 x, f32 y);
void write(std::ostream & os, f32x2 const & p);

void read(std::istream & is, f32x2 & p);
void read(std::istream & is, f32x2 * buffer, u32 length);

std::ostream & operator<<(std::ostream & os, f32x2 const & p);
std::istream & operator>>(std::istream & is, f32x2 & p);

}  // namespace gt
