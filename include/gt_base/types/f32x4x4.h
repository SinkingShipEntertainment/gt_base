
#pragma once

#include "gt_base/types/f32x3x3.h"
#include <json.hpp>

/// r0c0 r0c1 r0c2 r0c3
/// r1c0 r1c1 r1c2 r1c3
/// r2c0 r2c1 r2c2 r2c3
/// r3c0 r3c1 r3c2 r3c3

namespace gt
{

class f32x2;
class f32x3;
class f32x4;
class f32x3x3;

class f32x4x4
{
 public:
  f32x4x4();

  f32x4x4(f32 r0c0,
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
          f32 r3c3);

  f32x4x4(f32x3x3 const & m);

  f32x4x4(f32x4x4 const & m);

  f32x4x4(nlohmann::json const & j);

  f32x4x4(f32 const * p);

  /// setting
  void set(nlohmann::json const & j);

  void set(f32 const * p);

  void setToIdentity();

  void scale(f32 x, f32 y);

  void scale(f32x2 const & p);

  void scale(const f32x3 & p);

	// template <typename T> void scale(T const & s) {return T2(v1.x * v1.x,  v2.y * v2.y);}

  void rotate(const f32x3x3 & p);

  void rotate(const f32x4 & q);

  void translate(f32 x, f32 y);

  void translate(const f32x2 & p);

  void translate(const f32x3 & p);

  void translate(f32 x, f32 y, f32 z);

  /// TODO: dangerous but useful?
  void setRow(u8 i, const f32x3 & row);

  void setRow(u8 i, const f32x4 & row);

  void setColumn(u8 j, const f32x4 & column);

  /// getting
  f32x3 const getScale() const;

  f32x3x3 const getRotation() const;

  f32x3 const getTranslate() const;

  f32x3 const direction() const;

  f32x4x4 const inverse() const;

  f32x4x4 const transpose() const;

  const f32 * memAddr() const { return &r0c0; }
  const f32x4 row(u8 i) const;

  const f32x4 column(u8 j) const;

  /// members
  f32 r0c0, r0c1, r0c2, r0c3, r1c0, r1c1, r1c2, r1c3, r2c0, r2c1, r2c2, r2c3,
      r3c0, r3c1, r3c2, r3c3;
};

f32x2 const operator*(f32x2 const & v, const f32x4x4 & m);
f32x3 const operator*(f32x3 const & v, const f32x4x4 & m);
f32x4 const operator*(f32x4 const & v, const f32x4x4 & m);
void operator*=(f32x3 & v, const f32x4x4 & m);

void operator*=(f32x4x4 & m1, f32 v2);
void operator*=(f32x4x4 & m1, f32x4x4 const & m2);
f32x4x4 const operator*(f32x4x4 const & m1, const f32x4x4 & m2);

/// io
void write(std::ostream & os, const f32x4x4 & matrix);

void read(std::istream & is, f32x4x4 & p);

std::ostream & operator<<(std::ostream & os, const f32x4x4 & p);

std::istream & operator>>(std::istream & is, f32x4x4 & p);

}  // namespace gt
