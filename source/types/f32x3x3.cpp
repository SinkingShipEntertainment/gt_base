
#include "gt_base/types/f32x3x3.h"
#include "gt_base/types/f32x2.h"
#include "gt_base/types/f32x3.h"
#include "gt_base/types/f32x4.h"

using namespace gt;
using namespace std;

/// r0c0 r0c1 r0c2
/// r1c0 r1c1 r1c2
/// r2c0 r2c1 r2c2

f32x3x3::f32x3x3()
{
  r0c0 = 1;
  r0c1 = 0;
  r0c2 = 0;

  r1c0 = 0;
  r1c1 = 1;
  r1c2 = 0;

  r2c0 = 0;
  r2c1 = 0;
  r2c2 = 1;
}

f32x3x3::f32x3x3(f32 r0c0,
                 f32 r0c1,
                 f32 r0c2,
                 f32 r1c0,
                 f32 r1c1,
                 f32 r1c2,
                 f32 r2c0,
                 f32 r2c1,
                 f32 r2c2)
{
  this->r0c0 = r0c0;
  this->r0c1 = r0c1;
  this->r0c2 = r0c2;

  this->r1c0 = r1c0;
  this->r1c1 = r1c1;
  this->r1c2 = r1c2;

  this->r2c0 = r2c0;
  this->r2c1 = r2c1;
  this->r2c2 = r2c2;
}

f32x3x3::f32x3x3(const f32x3 & r0, const f32x3 & r1, const f32x3 & r2)
{
  r0c0 = r0.x;
  r0c1 = r0.y;
  r0c2 = r0.z;

  r1c0 = r1.x;
  r1c1 = r1.y;
  r1c2 = r1.z;

  r2c0 = r2.x;
  r2c1 = r2.y;
  r2c2 = r2.z;
}

f32x3x3::f32x3x3(const f32x4 & q)  /// quaternion
{
  r0c0 = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
  r0c1 = 2.0f * (q.x * q.y + q.z * q.w);
  r0c2 = 2.0f * (q.x * q.z - q.y * q.w);

  r1c0 = 2.0f * (q.x * q.y - q.z * q.w);
  r1c1 = 1.0f - 2.0f * (q.x * q.x + q.z * q.z);
  r1c2 = 2.0f * (q.z * q.y + q.x * q.w);

  r2c0 = 2.0f * (q.x * q.z + q.y * q.w);
  r2c1 = 2.0f * (q.y * q.z - q.x * q.w);
  r2c2 = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
}

/// setting
void f32x3x3::scale(f32 x, f32 y)
{
  r0c0 = x;
  r1c1 = y;
}

void f32x3x3::scale(f32x2 const & v)
{
  r0c0 = v.x;
  r1c1 = v.y;
}

void f32x3x3::translate(f32 x, f32 y)
{
  r2c0 = x;
  r2c1 = y;
}

void f32x3x3::setRow(u8 i, const f32x3 & r)
{
  i *= 3;
  f32 * memAddr = &r0c0;
  memAddr[i] = r.x;
  memAddr[i + 1] = r.y;
  memAddr[i + 2] = r.z;
}

void f32x3x3::setColumn(u8 j, const f32x3 & c)
{
  f32 * memAddr = &r0c0;
  memAddr[j] = c.x;
  memAddr[j + 3] = c.y;
  memAddr[j + 6] = c.z;
}

/// getting
const f32x3 f32x3x3::direction() const
{
  return f32x3(r2c0, r2c1, r2c2);
}

const f32x3x3 f32x3x3::transpose() const
{
  f32x3x3 r;

  r.r0c0 = r0c0;
  r.r0c1 = r1c0;
  r.r0c2 = r2c0;

  r.r1c0 = r0c1;
  r.r1c1 = r1c1;
  r.r1c2 = r2c1;

  r.r2c0 = r0c2;
  r.r2c1 = r1c2;
  r.r2c2 = r2c2;

  return r;
}

const f32x3 f32x3x3::row(u8 i) const
{
  i *= 3;
  const f32 * memAddr = &r0c0;
  return f32x3(memAddr[i], memAddr[i + 1], memAddr[i + 2]);
}

const f32x3 f32x3x3::column(u8 j) const
{
  const f32 * memAddr = &r0c0;
  return f32x3(memAddr[j], memAddr[j + 3], memAddr[j + 6]);
}

/// operators
f32x2 const gt::operator*(f32x2 const & v, f32x3x3 const & m)
{
  /// v implicitely becomes f32x3(v.x, v.y, 1)
  return f32x2(v.x * m.r0c0 + v.y * m.r1c0 + m.r2c0,
               v.x * m.r0c1 + v.y * m.r1c1 + m.r2c1);
}

f32x3 const gt::operator*(f32x3 const & v, f32x3x3 const & m)
{
  return f32x3(v.x * m.r0c0 + v.y * m.r1c0 + v.z * m.r2c0,
               v.x * m.r0c1 + v.y * m.r1c1 + v.z * m.r2c1,
               v.x * m.r0c2 + v.y * m.r1c2 + v.z * m.r2c2);
}

f32x3 const gt::operator*(f32x3x3 const & m, f32x3 const & v)
{
  // return f32x3(m.r0c0 * v.x + m.r0c1 * v.y + m.r0c2 * v.z,
  //              m.r1c0 * v.x + m.r1c1 * v.y + m.r1c2 * v.z,
  //              m.r2c0 * v.x + m.r2c1 * v.y + m.r2c2 * v.z);

  return f32x3(m.r0c0 * v.x + m.r1c0 * v.y + m.r2c0 * v.z,
               m.r0c1 * v.x + m.r1c1 * v.y + m.r2c1 * v.z,
               m.r0c2 * v.x + m.r1c2 * v.y + m.r2c2 * v.z);
}

void gt::operator*=(f32x3 & v, f32x3x3 const & m)
{
  f32x3 r;
  r.x = v.x * m.r0c0 + v.y * m.r1c0 + v.z * m.r2c0;
  r.y = v.x * m.r0c1 + v.y * m.r1c1 + v.z * m.r2c1;
  r.z = v.x * m.r0c2 + v.y * m.r1c2 + v.z * m.r2c2;
  v = r;
}

const f32x3x3 gt::operator*(const f32x3x3 & m1, const f32x3x3 & m2)
{
  f32x3x3 r;

  r.r0c0 = m1.r0c0 * m2.r0c0 + m1.r0c1 * m2.r0c0 + m1.r0c2 * m2.r0c0;
  r.r0c1 = m1.r0c0 * m2.r0c1 + m1.r0c1 * m2.r0c1 + m1.r0c2 * m2.r0c1;
  r.r0c2 = m1.r0c0 * m2.r0c2 + m1.r0c1 * m2.r0c2 + m1.r0c2 * m2.r0c2;

  r.r1c0 = m1.r1c0 * m2.r0c0 + m1.r1c1 * m2.r0c0 + m1.r1c2 * m2.r0c0;
  r.r1c1 = m1.r1c0 * m2.r0c1 + m1.r1c1 * m2.r0c1 + m1.r1c2 * m2.r0c1;
  r.r1c2 = m1.r1c0 * m2.r0c2 + m1.r1c1 * m2.r0c2 + m1.r1c2 * m2.r0c2;

  r.r2c0 = m1.r2c0 * m2.r0c0 + m1.r2c1 * m2.r0c0 + m1.r2c2 * m2.r0c0;
  r.r2c1 = m1.r2c0 * m2.r0c1 + m1.r2c1 * m2.r0c1 + m1.r2c2 * m2.r0c1;
  r.r2c2 = m1.r2c0 * m2.r0c2 + m1.r2c1 * m2.r0c2 + m1.r2c2 * m2.r0c2;

  return r;
}

void gt::operator*=(f32x3x3 & m1, const f32x3x3 & m2)
{
  m1.r0c0 = m1.r0c0 * m2.r0c0 + m1.r0c1 * m2.r0c0 + m1.r0c2 * m2.r0c0;
  m1.r0c1 = m1.r0c0 * m2.r0c1 + m1.r0c1 * m2.r0c1 + m1.r0c2 * m2.r0c1;
  m1.r0c2 = m1.r0c0 * m2.r0c2 + m1.r0c1 * m2.r0c2 + m1.r0c2 * m2.r0c2;

  m1.r1c0 = m1.r1c0 * m2.r0c0 + m1.r1c1 * m2.r0c0 + m1.r1c2 * m2.r0c0;
  m1.r1c1 = m1.r1c0 * m2.r0c1 + m1.r1c1 * m2.r0c1 + m1.r1c2 * m2.r0c1;
  m1.r1c2 = m1.r1c0 * m2.r0c2 + m1.r1c1 * m2.r0c2 + m1.r1c2 * m2.r0c2;

  m1.r2c0 = m1.r2c0 * m2.r0c0 + m1.r2c1 * m2.r0c0 + m1.r2c2 * m2.r0c0;
  m1.r2c1 = m1.r2c0 * m2.r0c1 + m1.r2c1 * m2.r0c1 + m1.r2c2 * m2.r0c1;
  m1.r2c2 = m1.r2c0 * m2.r0c2 + m1.r2c1 * m2.r0c2 + m1.r2c2 * m2.r0c2;
}

/// io
ostream & gt::operator<<(ostream & os, const f32x3x3 & p)
{
  os << p.row(0) << "\n" << p.row(1) << "\n" << p.row(2);
  return os;
}

istream & gt::operator>>(istream & is, f32x3x3 & p)
{
  f32x3 r;

  for (unsigned i = 0; i < 3; i++)
  {
    is >> r;
    p.setRow(i, r);
  }

  return is;
}
