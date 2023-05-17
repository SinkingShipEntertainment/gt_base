
#pragma once


#include "gt_base/types.h"
#include <istream>
#include <ostream>
#include <math.h>

namespace gt
{
class f32x3
{
 public:
  f32x3(f32 s = 0);
  
  f32x3(f32 x, f32 y, f32 z);

  template <class V, class S>
  f32x3(V const & v, S s)
  {
    this->x = v.x;
    this->y = v.y;
    this->z = s;
  }

  void set(f32 x, f32 y, f32 z);

  void normalize();

  /// getting
  f32 operator[](u32 i) const;

  f32 length() const;

  f32 minComponent() const;
  f32 maxComponent() const;

  const f32x3 normalized() const;

  const f32 * memAddr() const { return &x; }
  // f32x2 const xy() const { return f32x2(x, y); }
  f32 x, y, z;
};

f32x3 const operator+(f32x3 const & v1, const f32x3 & v2);
f32x3 const operator+(f32x3 const & v1, const f64 & v2);

f32x3 const operator-(f32x3 const & v1, const f32x3 & v2);
f32x3 const operator-(f32 const p1, const f32x3 & p2);
f32x3 const operator-(f32x3 const & v);

const f32x3 operator*(const f32x3 & v1, const f32x3 & v2);
const f32x3 operator*(const f32x3 & v, f32 s);

const f32x3 operator/(const f32x3 & v1, const f32x3 & v2);
const f32x3 operator/(const f32x3 & v, f32 s);

void operator+=(f32x3 & v1, const f32x3 & v2);
// void operator+=(f32x3 & v1, f32x2 const & v2);

void operator-=(f32x3 & v1, const f32x3 & v2);

void operator*=(f32x3 & v, f32 s);

void operator*=(f32x3 & v1, const f32x3 & v2);

void operator/=(f32x3 & v1, const f32x3 & v2);

void operator/=(f32x3 & v, f32 s);

bool operator>(const f32x3 & p1, const f32x3 & p2);
bool operator<(const f32x3 & p1, const f32x3 & p2);
bool operator>(const f32x3 & p1, f32 p2);
bool operator==(const f32x3 & v1, const f32x3 & v2);
bool operator!(const f32x3 & p1);

f32 length(const f32x3 & v1, const f32x3 & v2);

f32x3 normalize(const f32x3 & v1);

f32 dot(const f32x3 & v1, const f32x3 & v2);

const f32x3 cross(const f32x3 & v1, const f32x3 & v2);

const f32x3 average(const f32x3 & v1, const f32x3 & v2);

f64 angle(const f32x3 & v1, const f32x3 & v2);

/// io
void write(std::ostream & os, f32 x, f32 y, f32 z);
void write(std::ostream & os, const f32x3 & v);

void read(std::istream & is, f32x3 & p);
void read(std::istream & is, f32x3 * buffer, u32 length);

std::ostream & operator<<(std::ostream & os, const f32x3 & p);
std::istream & operator>>(std::istream & is, f32x3 & p);

}  // namespace gt
