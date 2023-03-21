
#include "gt_base/types/f32x2.h"

using namespace gt;
using namespace std;

f32x2::f32x2(f32 s)
{
  x = s;
  y = s;
}

f32x2::f32x2(f32 x, f32 y)
{
  this->x = x;
  this->y = y;
}

void f32x2::set(f32 x, f32 y)
{
  this->x = x;
  this->y = y;
}

f32 f32x2::length() const
{
  return sqrt(x * x + y * y);
}


f32 gt::dot(const f32x2 & p1, const f32x2 & p2)
{
  return f32(p1.x * p2.x + p1.y * p2.y);
}

f32x2 gt::operator+(f32x2 const & p1, f32 const & p2)
{
  return f32x2(p1.x + p2, p1.y + p2);
}

f32x2 gt::operator+(f32x2 const & p1, f32x2 const & p2)
{
  return f32x2(p1.x + p2.x, p1.y + p2.y);
}

void gt::operator+=(f32x2 & p1, f32x2 const & p2)
{
  p1.x += p2.x;
  p1.y += p2.y;
}

f32x2 gt::operator-(f32x2 const & p1, f32x2 const & p2)
{
  return f32x2(p1.x - p2.x, p1.y - p2.y);
}

void gt::operator-=(f32x2 & p1, f32x2 const & p2)
{
  p1.x -= p2.x;
  p1.y -= p2.y;
}

f32x2 gt::operator*(const f32x2 & p1, const f32 & p2)
{
  f32x2 r;
  r.x = p1.x * p2;
  r.y = p1.y * p2;
  return r;
}

f32x2 gt::operator*(const f32 & p1, const f32x2 & p2)
{
  f32x2 r;
  r.x = p1 * p2.x;
  r.y = p1 * p2.y;
  return r;
}

f32x2 gt::operator*(f32x2 const & p1, f32x2 const & p2)
{
  return f32x2(p1.x * p2.x, p1.y * p2.y);
}

void gt::operator*=(f32x2 & v1, f32 const & v2)
{
  v1.x *= v2;
  v1.y *= v2;
}

f32x2 gt::operator/(f32x2 const & p1, f32x2 const & p2)
{
  return f32x2(p1.x / p2.x, p1.y / p2.y);
}

f32x2 gt::operator/(f32x2 const & p1, f32 const & p2)
{
  return f32x2(p1.x / p2, p1.y / p2);
}

f32x2 gt::operator/(f32 p1, f32x2 const & p2)
{
  f32x2 r;
  r.x = p1 / p2.x;
  r.y = p1 / p2.y;
  return r;
}

bool gt::operator>(const f32x2 & p1, const f32x2 & p2)
{
  return p1.x > p2.x && p1.y > p2.y;
}

bool gt::operator<(const f32x2 & p1, const f32x2 & p2)
{
  return p1.x < p2.x && p1.y < p2.y;
}

bool gt::operator>(const f32x2 & p1, f32 p2)
{
  return p1.x > p2 && p1.y > p2;
}

bool gt::operator==(const f32x2 & p1, f32 p2)
{
  return p1.x == p2 && p1.y == p2;
}

bool gt::operator==(const f32x2 & p1, const f32x2 & p2)
{
  return p1.x == p2.x && p1.y == p2.y;
}

bool gt::operator!=(const f32x2 & p1, const f32x2 & p2)
{
  if (p1.x == p2.x && p1.y == p2.y)
    return false;
  return true;
}

bool gt::operator!(f32x2 const & v)
{
  if (v.x || v.y)
    return false;
  return true;
}

f32 gt::length(f32x2 const & v1, f32x2 const & v2)
{
  return (v2 - v1).length();
}

/// io
void gt::write(ostream & os, f32 x, f32 y)
{
  f32x2 v(x, y);
  os.write(reinterpret_cast<i8 *>(&v), sizeof(f32x2));
}

void gt::write(ostream & os, const f32x2 & p)
{
  os.write(reinterpret_cast<const i8 *>(&p), sizeof(f32x2));
}

void gt::read(istream & is, f32x2 & p)
{
  is.read(reinterpret_cast<i8 *>(&p), sizeof(f32x2));
}

void gt::read(istream & is, f32x2 * buffer, u32 length)
{
  is.read(reinterpret_cast<i8 *>(buffer), sizeof(f32x2) * length);
}

ostream & gt::operator<<(ostream & os, const f32x2 & p)
{
  os << p.x << ", " << p.y;
  return os;
}

istream & gt::operator>>(istream & is, f32x2 & p)
{
  i8 c;
  is >> p.x >> c >> p.y;
  return is;
}
