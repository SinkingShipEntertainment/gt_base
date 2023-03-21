
#include "gt_base/types/f32x3.h"

using namespace gt;
using namespace std;

f32x3::f32x3(f32 s)
{
  x = s;
  y = s;
  z = s;
}

f32x3::f32x3(f32 x, f32 y, f32 z)
{
  this->x = x;
  this->y = y;
  this->z = z;
}

void f32x3::set(f32 x, f32 y, f32 z)
{
  this->x = x;
  this->y = y;
  this->z = z;
}

f32 f32x3::operator[](u32 i) const
{
  // if(i > 2) return throw std::exception("f32x3[] index out of range");

  if(i == 0) return x;
  else if(i == 1) return y;
  else if(i == 2) return z;

  /// TODO: try something like
  // return this[sizeof(f32) * i];
}


f32 f32x3::length() const
{
  return sqrt(x * x + y * y + z * z);
}

void f32x3::normalize()
{
  f32 len = this->length();
  x /= len;
  y /= len;
  z /= len;
}

const f32x3 f32x3::normalized() const
{
  f32x3 r;

  f32 len = this->length();
  r.x = x / len;
  r.y = y / len;
  r.z = z / len;

  return r;
}

f32 f32x3::minComponent() const
{
  f32 r = x < y ? x : y;
  r = r < z ? r : z;
  return r;
}

f32 f32x3::maxComponent() const
{
  f32 r = x > y ? x : y;
  r = r > z ? r : z;
  return r;
}

const f32x3 gt::operator+(const f32x3 & v1, const f32x3 & v2)
{
  return f32x3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

const f32x3 gt::operator+(const f32x3 & v1, const f64 & v2)
{
  return f32x3(v1.x + v2, v1.y + v2, v1.z + v2);
}

const f32x3 gt::operator-(const f32x3 & v1, const f32x3 & v2)
{
  return f32x3(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

const f32x3 gt::operator-(f32 p1, const f32x3 & p2)
{
  return f32x3(p1 - p2.x, p1 - p2.y, p1 - p2.z);
}

const f32x3 gt::operator-(const f32x3 & v)
{
  return f32x3(-v.x, -v.y, -v.z);
}

const f32x3 gt::operator*(const f32x3 & v1, const f32x3 & v2)
{
  return f32x3(v1.x * v2.x, v1.x * v2.y, v1.x * v2.z);
}

const f32x3 gt::operator*(const f32x3 & v, f32 s)
{
  return f32x3(v.x * s, v.y * s, v.z * s);
}

const f32x3 gt::operator/(const f32x3 & v1, const f32x3 & v2)
{
  return f32x3(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z);
}

const f32x3 gt::operator/(const f32x3 & v, f32 s)
{
  return f32x3(v.x / s, v.y / s, v.z / s);
}

void gt::operator+=(f32x3 & v1, const f32x3 & v2)
{
  v1.x += v2.x;
  v1.y += v2.y;
  v1.z += v2.z;
}

// void gt::operator+=(f32x3 & v1, f32x2 const & v2)
// {
//   v1.x += v2.x;
//   v1.y += v2.y;
// }

void gt::operator-=(f32x3 & v1, const f32x3 & v2)
{
  v1.x -= v2.x;
  v1.y -= v2.y;
  v1.z -= v2.z;
}

void gt::operator*=(f32x3 & v1, const f32x3 & v2)
{
  v1.x *= v2.x;
  v1.y *= v2.y;
  v1.z *= v2.z;
}

void gt::operator*=(f32x3 & v, f32 s)
{
  v.x *= s;
  v.y *= s;
  v.z *= s;
}

void gt::operator/=(f32x3 & v1, const f32x3 & v2)
{
  v1.x /= v2.x;
  v1.y /= v2.y;
  v1.z /= v2.z;
}

void gt::operator/=(f32x3 & v, f32 s)
{
  v.x /= s;
  v.y /= s;
  v.z /= s;
}

bool gt::operator>(const f32x3 & p1, const f32x3 & p2)
{
  return p1.x > p2.x && p1.y > p2.y;
}

bool gt::operator<(const f32x3 & p1, const f32x3 & p2)
{
  return p1.x < p2.x && p1.y < p2.y;
}

bool gt::operator>(const f32x3 & p1, f32 p2)
{
  return p1.x > p2 && p1.y > p2;
}

bool gt::operator==(const f32x3 & v1, const f32x3 & v2)
{
  if (v1.x == v2.x && v1.y == v2.y && v1.z == v2.z)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool gt::operator!(const f32x3 & p1)
{
  return !p1.x && !p1.y;
}

f32 gt::length(const f32x3 & v1, const f32x3 & v2)
{
  return (v2 - v1).length();
}

f32x3 gt::normalize(const f32x3 & v)
{
  return v.normalized();
}

f32 gt::dot(const f32x3 & v1, const f32x3 & v2)
{
  return f32(v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
}

const f32x3 gt::cross(const f32x3 & v1, const f32x3 & v2)
{
  return f32x3(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z,
               v1.x * v2.y - v1.y * v2.x);
}

const f32x3 gt::average(const f32x3 & v1, const f32x3 & v2)
{
  return f32x3((v1.x + v2.x) * 0.5, (v1.y + v2.y) * 0.5, (v1.z + v2.z) * 0.5);
}

f64 gt::angle(const f32x3 & v1, const f32x3 & v2)
{
  /// get the angle in radians between the 2 points
  f64 angle = acos(dot(v1, v2) / v1.length() * v2.length());

  /// we make sure that the angle is not a -1.#IND0000000 number, which means
  /// indefinite
  // if(_isnan(angle))
  // {
  // 	return 0;
  // }
  return angle;
}

/// io
void gt::write(ostream & os, f32 x, f32 y, f32 z)
{
  f32x3 v(x, y, z);
  os.write(reinterpret_cast<const i8 *>(&v), sizeof(f32x3));
}

void gt::write(ostream & os, const f32x3 & v)
{
  os.write(reinterpret_cast<const i8 *>(&v), sizeof(f32x3));
}

void gt::read(istream & is, f32x3 & p)
{
  is.read(reinterpret_cast<i8 *>(&p), sizeof(f32x3));
}

void gt::read(istream & is, f32x3 * buffer, u32 length)
{
  is.read(reinterpret_cast<i8 *>(buffer), sizeof(f32x3) * length);
}

ostream & gt::operator<<(ostream & os, const f32x3 & p)
{
  os << p.x << ", " << p.y << ", " << p.z;
  return os;
}

istream & gt::operator>>(istream & is, f32x3 & p)
{
  i8 c;
  is >> p.x >> c >> p.y >> c >> p.z;
  return is;
}
