
#include "gt_base/types/f32x4x4.h"
#include "gt_base/types/f32x2.h"
#include "gt_base/types/f32x3.h"
#include "gt_base/types/f32x4.h"
#include "gt_base/types/f32x3x3.h"

using namespace gt;
using namespace std;
using namespace nlohmann;

/// r0c0 r0c1 r0c2 r0c3
/// r1c0 r1c1 r1c2 r1c3
/// r2c0 r2c1 r2c2 r2c3
/// r3c0 r3c1 r3c2 r3c3

f32x4x4::f32x4x4()
{
  setToIdentity();
}

f32x4x4::f32x4x4(f32 r0c0,
                 f32 r0c1,
                 f32 r0c2,
                 f32 r0c3,
                 f32 r1c0,
                 f32 r1c1,
                 f32 r1c2,
                 f32 r1c3,
                 f32 r2c0,
                 f32 r2c1,
                 f32 r2c2,
                 f32 r2c3,
                 f32 r3c0,
                 f32 r3c1,
                 f32 r3c2,
                 f32 r3c3)
{
  this->r0c0 = r0c0;
  this->r0c1 = r0c1;
  this->r0c2 = r0c2;
  this->r0c3 = r0c3;

  this->r1c0 = r1c0;
  this->r1c1 = r1c1;
  this->r1c2 = r1c2;
  this->r1c3 = r1c3;

  this->r2c0 = r2c0;
  this->r2c1 = r2c1;
  this->r2c2 = r2c2;
  this->r2c3 = r2c3;

  this->r3c0 = r3c0;
  this->r3c1 = r3c1;
  this->r3c2 = r3c2;
  this->r3c3 = r3c3;
}

f32x4x4::f32x4x4(f32x3x3 const & m)
{
  this->r0c0 = m.r0c0;
  this->r0c1 = m.r0c1;
  this->r0c2 = m.r0c2;
  this->r0c3 = 0;

  this->r1c0 = m.r1c0;
  this->r1c1 = m.r1c1;
  this->r1c2 = m.r1c2;
  this->r1c3 = 0;

  this->r2c0 = m.r2c0;
  this->r2c1 = m.r2c1;
  this->r2c2 = m.r2c2;
  this->r2c3 = 0;

  this->r3c0 = 0;
  this->r3c1 = 0;
  this->r3c2 = 0;
  this->r3c3 = 1;
}

f32x4x4::f32x4x4(f32x4x4 const & m)
{
  this->r0c0 = m.r0c0;
  this->r0c1 = m.r0c1;
  this->r0c2 = m.r0c2;
  this->r0c3 = m.r0c3;

  this->r1c0 = m.r1c0;
  this->r1c1 = m.r1c1;
  this->r1c2 = m.r1c2;
  this->r1c3 = m.r1c3;

  this->r2c0 = m.r2c0;
  this->r2c1 = m.r2c1;
  this->r2c2 = m.r2c2;
  this->r2c3 = m.r2c3;

  this->r3c0 = m.r3c0;
  this->r3c1 = m.r3c1;
  this->r3c2 = m.r3c2;
  this->r3c3 = m.r3c3;
}

f32x4x4::f32x4x4(json const & j)
{
  set(j);
}

f32x4x4::f32x4x4(f32 const * p)
{
  set(p);
}

/// setting
void f32x4x4::set(json const & j)
{
  r0c0 = j[0];
  r0c1 = j[1];
  r0c2 = j[2];
  r0c3 = j[3];

  r1c0 = j[4];
  r1c1 = j[5];
  r1c2 = j[6];
  r1c3 = j[7];

  r2c0 = j[8];
  r2c1 = j[9];
  r2c2 = j[10];
  r2c3 = j[11];

  r3c0 = j[12];
  r3c1 = j[13];
  r3c2 = j[14];
  r3c3 = j[15];
}

void f32x4x4::set(f32 const * p)
{
  r0c0 = p[0];
  r0c1 = p[1];
  r0c2 = p[2];
  r0c3 = p[3];

  r1c0 = p[4];
  r1c1 = p[5];
  r1c2 = p[6];
  r1c3 = p[7];

  r2c0 = p[8];
  r2c1 = p[9];
  r2c2 = p[10];
  r2c3 = p[11];

  r3c0 = p[12];
  r3c1 = p[13];
  r3c2 = p[14];
  r3c3 = p[15];
}

void f32x4x4::setToIdentity()
{
  r0c0 = 1;
  r0c1 = 0;
  r0c2 = 0;
  r0c3 = 0;

  r1c0 = 0;
  r1c1 = 1;
  r1c2 = 0;
  r1c3 = 0;

  r2c0 = 0;
  r2c1 = 0;
  r2c2 = 1;
  r2c3 = 0;

  r3c0 = 0;
  r3c1 = 0;
  r3c2 = 0;
  r3c3 = 1;
}

void f32x4x4::scale(f32 x, f32 y)
{
  r0c0 = x;
  r1c1 = y;
}

void f32x4x4::scale(f32x2 const & p)
{
  r0c0 = p.x;
  r1c1 = p.y;
}

void f32x4x4::scale(const f32x3 & p)
{
  r0c0 = p.x;
  r1c1 = p.y;
  r2c2 = p.z;
}

void f32x4x4::rotate(const f32x3x3 & p)
{
  r0c0 = p.r0c0;
  r0c1 = p.r0c1;
  r0c2 = p.r0c2;

  r1c0 = p.r1c0;
  r1c1 = p.r1c1;
  r1c2 = p.r1c2;

  r2c0 = p.r2c0;
  r2c1 = p.r2c1;
  r2c2 = p.r2c2;
}

void f32x4x4::rotate(const f32x4 & q)
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

void f32x4x4::translate(f32 x, f32 y)
{
  r3c0 = x;
  r3c1 = y;
}

void f32x4x4::translate(const f32x2 & p)
{
  r3c0 = p.x;
  r3c1 = p.y;
}

void f32x4x4::translate(const f32x3 & p)
{
  r3c0 = p.x;
  r3c1 = p.y;
  r3c2 = p.z;
}

void f32x4x4::translate(f32 x, f32 y, f32 z)
{
  r3c0 = x;
  r3c1 = y;
  r3c2 = z;
}

/// TODO: dangerous but useful?
void f32x4x4::setRow(u8 i, const f32x3 & row)
{
  i *= 4;
  f32 * memAddr = &r0c0;
  memAddr[i] = row.x;
  memAddr[i + 1] = row.y;
  memAddr[i + 2] = row.z;
}

void f32x4x4::setRow(u8 i, const f32x4 & row)
{
  i *= 4;
  f32 * memAddr = &r0c0;
  memAddr[i] = row.x;
  memAddr[i + 1] = row.y;
  memAddr[i + 2] = row.z;
  memAddr[i + 3] = row.w;
}

void f32x4x4::setColumn(u8 j, const f32x4 & column)
{
  f32 * memAddr = &r0c0;
  memAddr[j] = column.x;
  memAddr[j + 4] = column.y;
  memAddr[j + 8] = column.z;
  memAddr[j + 12] = column.w;
}

/// getting
f32x3 const f32x4x4::getScale() const
{
  return f32x3(r0c0, r1c1, r2c2);
}

f32x3x3 const f32x4x4::getRotation() const
{
  return f32x3x3(r0c0, r0c1, r0c2, r1c0, r1c1, r1c2, r2c0, r2c1, r2c2);
}

f32x3 const f32x4x4::getTranslate() const
{
  return f32x3(r3c0, r3c1, r3c2);
}

f32x3 const f32x4x4::direction() const
{
  return f32x3(r2c0, r2c1, r2c2);
}

f32x4x4 const f32x4x4::inverse() const
{
  f32x4x4 r;
  f32 const * m = &r0c0;
  f32 * inv = &r.r0c0;

  inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] +
           m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];

  inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] -
           m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];

  inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] +
           m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];

  inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] -
            m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];

  inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] -
           m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];

  inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] +
           m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];

  inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] -
           m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];

  inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] +
            m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];

  inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] +
           m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];

  inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] -
           m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];

  inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] +
            m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];

  inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] -
            m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];

  inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] -
           m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];

  inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] +
           m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];

  inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] -
            m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];

  inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] +
            m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

  f32 det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

  if (det == 0)
    return f32x4x4();  // error(str("f32x4x4::inverse: determinant is 0"));//

  det = 1.0 / det;

  for (u8 i = 0; i < 16; i++)
    inv[i] *= det;

  return r;
}

f32x4x4 const f32x4x4::transpose() const
{
  return f32x4x4(r0c0, r1c0, r2c0, r3c0, r0c1, r1c1, r2c1, r3c1, r0c2, r1c2,
                 r2c2, r3c2, r0c3, r1c3, r2c3, r3c3);
}

const f32x4 f32x4x4::row(u8 i) const
{
  i *= 4;
  const f32 * memAddr = &r0c0;
  return f32x4(memAddr[i], memAddr[i + 1], memAddr[i + 2], memAddr[i + 3]);
}

const f32x4 f32x4x4::column(u8 j) const
{
  const f32 * memAddr = &r0c0;
  return f32x4(memAddr[j], memAddr[j + 4], memAddr[j + 8], memAddr[j + 12]);
}

/// operators
f32x2 const gt::operator*(f32x2 const & v, f32x4x4 const & m)
{
  /// v implicitely becomes f32x4(v.x, v.y, 0, 1)

  return f32x2(v.x * m.r0c0 + v.y * m.r1c0 + m.r3c0,
               v.x * m.r0c1 + v.y * m.r1c1 + m.r3c1);
}

f32x3 const gt::operator*(f32x3 const & v, f32x4x4 const & m)
{
  /// v implicitely becomes f32x4(v.x, v.y, v.z, 1)

  return f32x3(v.x * m.r0c0 + v.y * m.r1c0 + v.z * m.r2c0 + m.r3c0,
               v.x * m.r0c1 + v.y * m.r1c1 + v.z * m.r2c1 + m.r3c1,
               v.x * m.r0c2 + v.y * m.r1c2 + v.z * m.r2c2 + m.r3c2);
}

f32x4 const gt::operator*(f32x4 const & v, f32x4x4 const & m)
{
  return f32x4(v.x * m.r0c0 + v.y * m.r1c0 + v.z * m.r2c0 + v.w * m.r3c0,
               v.x * m.r0c1 + v.y * m.r1c1 + v.z * m.r2c1 + v.w * m.r3c1,
               v.x * m.r0c2 + v.y * m.r1c2 + v.z * m.r2c2 + v.w * m.r3c2,
               v.x * m.r0c3 + v.y * m.r1c3 + v.z * m.r2c3 + v.w * m.r3c3);
}

void gt::operator*=(f32x3 & v, const f32x4x4 & m)
{
  /// v implicitely becomes f32x4(v.x, v.y, v.z, 1)
  f32x3 r;
  r.x = v.x * m.r0c0 + v.y * m.r1c0 + v.z * m.r2c0 + m.r3c0;
  r.y = v.x * m.r0c1 + v.y * m.r1c1 + v.z * m.r2c1 + m.r3c1;
  r.z = v.x * m.r0c2 + v.y * m.r1c2 + v.z * m.r2c2 + m.r3c2;
  v = r;
}

void gt::operator*=(f32x4x4 & m1, f32 v2)
{
  m1.r0c0 *= v2;
  m1.r0c1 *= v2;
  m1.r0c2 *= v2;
  m1.r0c3 *= v2;

  m1.r1c0 *= v2;
  m1.r1c1 *= v2;
  m1.r1c2 *= v2;
  m1.r1c3 *= v2;

  m1.r2c0 *= v2;
  m1.r2c1 *= v2;
  m1.r2c2 *= v2;
  m1.r2c3 *= v2;

  m1.r3c0 *= v2;
  m1.r3c1 *= v2;
  m1.r3c2 *= v2;
  m1.r3c3 *= v2;
}

void gt::operator*=(f32x4x4 & m1, f32x4x4 const & m2)
{
  f32x4x4 r;

  r.r0c0 = m1.r0c0 * m2.r0c0 + m1.r0c1 * m2.r1c0 + m1.r0c2 * m2.r2c0 +
           m1.r0c3 * m2.r3c0;
  r.r0c1 = m1.r0c0 * m2.r0c1 + m1.r0c1 * m2.r1c1 + m1.r0c2 * m2.r2c1 +
           m1.r0c3 * m2.r3c1;
  r.r0c2 = m1.r0c0 * m2.r0c2 + m1.r0c1 * m2.r1c2 + m1.r0c2 * m2.r2c2 +
           m1.r0c3 * m2.r3c2;
  r.r0c3 = m1.r0c0 * m2.r0c3 + m1.r0c1 * m2.r1c3 + m1.r0c2 * m2.r2c3 +
           m1.r0c3 * m2.r3c3;

  r.r1c0 = m1.r1c0 * m2.r0c0 + m1.r1c1 * m2.r1c0 + m1.r1c2 * m2.r2c0 +
           m1.r1c3 * m2.r3c0;
  r.r1c1 = m1.r1c0 * m2.r0c1 + m1.r1c1 * m2.r1c1 + m1.r1c2 * m2.r2c1 +
           m1.r1c3 * m2.r3c1;
  r.r1c2 = m1.r1c0 * m2.r0c2 + m1.r1c1 * m2.r1c2 + m1.r1c2 * m2.r2c2 +
           m1.r1c3 * m2.r3c2;
  r.r1c3 = m1.r1c0 * m2.r0c3 + m1.r1c1 * m2.r1c3 + m1.r1c2 * m2.r2c3 +
           m1.r1c3 * m2.r3c3;

  r.r2c0 = m1.r2c0 * m2.r0c0 + m1.r2c1 * m2.r1c0 + m1.r2c2 * m2.r2c0 +
           m1.r2c3 * m2.r3c0;
  r.r2c1 = m1.r2c0 * m2.r0c1 + m1.r2c1 * m2.r1c1 + m1.r2c2 * m2.r2c1 +
           m1.r2c3 * m2.r3c1;
  r.r2c2 = m1.r2c0 * m2.r0c2 + m1.r2c1 * m2.r1c2 + m1.r2c2 * m2.r2c2 +
           m1.r2c3 * m2.r3c2;
  r.r2c3 = m1.r2c0 * m2.r0c3 + m1.r2c1 * m2.r1c3 + m1.r2c2 * m2.r2c3 +
           m1.r2c3 * m2.r3c3;

  r.r3c0 = m1.r3c0 * m2.r0c0 + m1.r3c1 * m2.r1c0 + m1.r3c2 * m2.r2c0 +
           m1.r3c3 * m2.r3c0;
  r.r3c1 = m1.r3c0 * m2.r0c1 + m1.r3c1 * m2.r1c1 + m1.r3c2 * m2.r2c1 +
           m1.r3c3 * m2.r3c1;
  r.r3c2 = m1.r3c0 * m2.r0c2 + m1.r3c1 * m2.r1c2 + m1.r3c2 * m2.r2c2 +
           m1.r3c3 * m2.r3c2;
  r.r3c3 = m1.r3c0 * m2.r0c3 + m1.r3c1 * m2.r1c3 + m1.r3c2 * m2.r2c3 +
           m1.r3c3 * m2.r3c3;

  m1 = r;
}

f32x4x4 const gt::operator*(f32x4x4 const & m1, f32x4x4 const & m2)
{
  return f32x4x4(m1.r0c0 * m2.r0c0 + m1.r0c1 * m2.r1c0 + m1.r0c2 * m2.r2c0 +
                     m1.r0c3 * m2.r3c0,
                 m1.r0c0 * m2.r0c1 + m1.r0c1 * m2.r1c1 + m1.r0c2 * m2.r2c1 +
                     m1.r0c3 * m2.r3c1,
                 m1.r0c0 * m2.r0c2 + m1.r0c1 * m2.r1c2 + m1.r0c2 * m2.r2c2 +
                     m1.r0c3 * m2.r3c2,
                 m1.r0c0 * m2.r0c3 + m1.r0c1 * m2.r1c3 + m1.r0c2 * m2.r2c3 +
                     m1.r0c3 * m2.r3c3,

                 m1.r1c0 * m2.r0c0 + m1.r1c1 * m2.r1c0 + m1.r1c2 * m2.r2c0 +
                     m1.r1c3 * m2.r3c0,
                 m1.r1c0 * m2.r0c1 + m1.r1c1 * m2.r1c1 + m1.r1c2 * m2.r2c1 +
                     m1.r1c3 * m2.r3c1,
                 m1.r1c0 * m2.r0c2 + m1.r1c1 * m2.r1c2 + m1.r1c2 * m2.r2c2 +
                     m1.r1c3 * m2.r3c2,
                 m1.r1c0 * m2.r0c3 + m1.r1c1 * m2.r1c3 + m1.r1c2 * m2.r2c3 +
                     m1.r1c3 * m2.r3c3,

                 m1.r2c0 * m2.r0c0 + m1.r2c1 * m2.r1c0 + m1.r2c2 * m2.r2c0 +
                     m1.r2c3 * m2.r3c0,
                 m1.r2c0 * m2.r0c1 + m1.r2c1 * m2.r1c1 + m1.r2c2 * m2.r2c1 +
                     m1.r2c3 * m2.r3c1,
                 m1.r2c0 * m2.r0c2 + m1.r2c1 * m2.r1c2 + m1.r2c2 * m2.r2c2 +
                     m1.r2c3 * m2.r3c2,
                 m1.r2c0 * m2.r0c3 + m1.r2c1 * m2.r1c3 + m1.r2c2 * m2.r2c3 +
                     m1.r2c3 * m2.r3c3,

                 m1.r3c0 * m2.r0c0 + m1.r3c1 * m2.r1c0 + m1.r3c2 * m2.r2c0 +
                     m1.r3c3 * m2.r3c0,
                 m1.r3c0 * m2.r0c1 + m1.r3c1 * m2.r1c1 + m1.r3c2 * m2.r2c1 +
                     m1.r3c3 * m2.r3c1,
                 m1.r3c0 * m2.r0c2 + m1.r3c1 * m2.r1c2 + m1.r3c2 * m2.r2c2 +
                     m1.r3c3 * m2.r3c2,
                 m1.r3c0 * m2.r0c3 + m1.r3c1 * m2.r1c3 + m1.r3c2 * m2.r2c3 +
                     m1.r3c3 * m2.r3c3);
}

/// io
void gt::write(ostream & os, const f32x4x4 & p)
{
  os.write(reinterpret_cast<const i8 *>(&p), sizeof(f32x4x4));
}

void gt::read(istream & is, f32x4x4 & p)
{
  is.read(reinterpret_cast<i8 *>(&p), sizeof(f32x4x4));
}

ostream & gt::operator<<(ostream & os, const f32x4x4 & p)
{
  os << p.row(0) << "\n" << p.row(1) << "\n" << p.row(2) << "\n" << p.row(3);
  return os;
}

istream & gt::operator>>(istream & is, f32x4x4 & p)
{
  f32x4 r;

  for (u8 i = 0; i < 4; i++)
  {
    is >> r;
    p.setRow(i, r);
  }

  return is;
}
