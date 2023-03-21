
#include "gt_base/types/u8x3.h"

using namespace gt;
using namespace std;

u8x3::u8x3(u8 s)
{
  x = s;
  y = s;
  z = s;
}

u8x3::u8x3(u8 x, u8 y, u8 z)
{
  this->x = x;
  this->y = y;
  this->z = z;
}

/// io
void read(istream & is, u8x3 & p)
{
  is.read(reinterpret_cast<i8 *>(&p), sizeof(u8x3));
}

ostream & gt::operator<<(ostream & os, const u8x3 & p)
{
  os << static_cast<u16>(p.x) << ", " << static_cast<u16>(p.y) << ", "
     << static_cast<u16>(p.z);
  return os;
}

istream & gt::operator>>(istream & is, u8x3 & p)
{
  i8 comma;
  is >> p.x >> comma >> p.y >> comma >> p.z;
  return is;
}
