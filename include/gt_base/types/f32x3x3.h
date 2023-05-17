
#pragma once

#include "gt_base/types.h"
#include <istream>
#include <ostream>

/// r0c0 r0c1 r0c2
/// r1c0 r1c1 r1c2
/// r2c0 r2c1 r2c2

namespace gt
{
  class f32x2;
  class f32x3;
  class f32x4;

  class f32x3x3
  {
  public:
    f32x3x3();
    f32x3x3(f32 r0c0, f32 r0c1, f32 r0c2,
            f32 r1c0, f32 r1c1, f32 r1c2,
            f32 r2c0, f32 r2c1, f32 r2c2);
    f32x3x3(const f32x3 &r0, const f32x3 &r1, const f32x3 &r2);
    f32x3x3(const f32x4 &q); /// quaternion

    /// setting
    void scale(f32 x, f32 y);
    void scale(f32x2 const &v);
    void translate(f32 x, f32 y);

    void setRow(u8 i, const f32x3 &r);
    void setColumn(u8 j, const f32x3 &c);

    /// getting
    f32x3 const direction() const;
    f32x3x3 const transpose() const;
    f32 const *memAddr() const { return &r0c0; }

    f32x3 const row(u8 i) const;
    f32x3 const column(u8 j) const;

    /// members
    f32 r0c0, r0c1, r0c2,
        r1c0, r1c1, r1c2,
        r2c0, r2c1, r2c2;
  };

  f32x2 const operator*(f32x2 const &v, f32x3x3 const &m);
  f32x3 const operator*(f32x3 const &v, f32x3x3 const &m);
  f32x3 const operator*(f32x3x3 const &m, f32x3 const &v);
  void operator*=(f32x3 &v, f32x3x3 const &m);

  f32x3x3 const operator*(f32x3x3 const &m1, f32x3x3 const &m2);
  void operator*=(f32x3x3 &m1, f32x3x3 const &m2);

  /// io
  std::ostream &operator<<(std::ostream &os, const f32x3x3 &p);
  std::istream &operator>>(std::istream &is, f32x3x3 &p);

} // namespace gt
