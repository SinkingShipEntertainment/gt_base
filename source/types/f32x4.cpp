
#include "gt_base/types/f32x4.h"
#include "gt_base/types/f32x2.h"
#include "gt_base/types/f32x3.h"

using namespace gt;
using namespace std;

f32x4::f32x4(f32 s) 
{
  x = s; 
  y = s; 
  z = s;
  w = s;
}

f32x4::f32x4(f32 x, f32 y, f32 z, f32 w)
{
    this->x = x; 
    this->y = y; 
    this->z = z;
    this->w = w;
}

f32x4::f32x4(const f32x2 & p1, const f32x2 & p2)
{
    x = p1.x; 
    y = p1.y; 
    z = p2.x;
    w = p2.y;
}

f32x4::f32x4(f32x2 const & v, f32 z, f32 w)
{
    x = v.x; 
    y = v.y; 
    this->z = z;
    this->w = w;
}

f32x4::f32x4(const f32x3 & v, f32 w)
{
    x = v.x; 
    y = v.y; 
    z = v.z;
    this->w = w;
}

f32x2 const f32x4::xy()  const {return f32x2(x, y);}

f32x3 const f32x4::xyz() const {return f32x3(x, y, z);}

f32 f32x4::length(const f32x4 & v) const
{
    return sqrt(x * x + y * y + z * z + w * w);
}

void f32x4::normalize()
{
    f32 len = length(*this);
    x /= len;
    y /= len;
    z /= len;
    w /= len;
}

const f32x4 f32x4::normalized() const
{
    f32x4 r;
    
    f32 len = length(*this);

    r.x /= len;
    r.y /= len;
    r.z /= len;
    r.w /= len;
    
    return r;
}

bool f32x4::hasInside(const f32x2 & p) const
{
    // return p.x > x && p.y > y && p.x < z && p.y < w;
    return p.x >= x && p.y >= y && p.x <= z && p.y <= w;
}

const f32x4 gt::operator + (const f32x4 & v1, const f32x4 & v2)
{
    return f32x4(v1.x + v2.x , v1.y + v2.y, v1.z + v2.z, v1.w + v2.w);
}

const f32x4 gt::operator + (const f32x4 & v1, const f32x3 & v2)
{
    return f32x4(v1.x + v2.x , v1.y + v2.y, v1.z + v2.z, v1.w);
}

const f32x4 gt::operator - (const f32x4 & v1, const f32x4 & v2)
{
    return f32x4(v1.x - v2.x , v1.y - v2.y, v1.z - v2.z, v1.w - v2.w);
}

const f32x4 gt::operator - (f32 p1, const f32x4 & p2)
{
    return f32x4(p1 - p2.x, p1 - p2.y, p1 - p2.z, p1 - p2.w);
}

f32x4 const gt::operator - (const f32x4 & v)
{
    return f32x4(-v.x, -v.y, -v.z, -v.w);
}

f32x4 const gt::operator * (f32x4 const & v1, f32 const & v2)
{
    return f32x4(v1.x * v2 , v1.y * v2, v1.z * v2, v1.w * v2);
}

const f32x4 gt::operator * (const f32x4 & v1, const f32x4 & v2)
{
    return f32x4(v1.x * v2.x , v1.y * v2.y, v1.z * v2.z, v1.w * v2.w);
}

const f32x4 gt::operator / (const f32x4 & v1, const f32x4 & v2)
{
    return f32x4(v1.x / v2.x , v1.y / v2.y, v1.z / v2.z, v1.w / v2.w);
}

void gt::operator += (f32x4 & v1, const f32x4 & v2)
{
    v1.x += v2.x;
    v1.y += v2.y;
    v1.z += v2.z;
    v1.w += v2.w;
}

void gt::operator -= (f32x4 & v1, const f32x4 & v2)
{
    v1.x -= v2.x;
    v1.y -= v2.y;
    v1.z -= v2.z;
    v1.w -= v2.w;
}

void gt::operator *= (f32x4 & v1, const f32x4 & v2)
{
    v1.x *= v2.x;
    v1.y *= v2.y;
    v1.z *= v2.z;
    v1.w *= v2.w;
}

void gt::operator *= (f32x4 & v, f32 s)
{
    v.x *= s;
    v.y *= s;
    v.z *= s;
    v.w *= s;
}

void gt::operator /= (f32x4 & v1, const f32x4 & v2)
{
    v1.x /= v2.x;
    v1.y /= v2.y;
    v1.z /= v2.z;
    v1.w /= v2.w;
}

void gt::operator /= (f32x4 & v, f32 s)
{
    v.x /= s;
    v.y /= s;
    v.z /= s;
    v.w /= s;
}

bool gt::operator == (const f32x4 & v1, const f32x4 & v2)
{
    if(v1.x == v2.x && v1.y == v2.y && v1.z == v2.z && v1.w == v2.w)
    {
        return true;
    }
    else
    {
        return false;
    }
}

f32 gt::dot(const f32x4 & v1, const f32x4 & v2)
{
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}

/// io
void gt::write(ostream & os, const f32x4 & v)
{
    os.write(reinterpret_cast<const i8*>(&v), sizeof(f32x4));
}

void gt::read(istream & is, f32x4 & p)
{
    is.read(reinterpret_cast<i8*>(&p), sizeof(f32x4));
}

ostream & gt::operator << (ostream & os, const f32x4 & p)
{
    os << p.x << ", " << p.y << ", " << p.z << ", " << p.w;
    return os;
}

std::istream & gt::operator >> (istream & is, f32x4 & p)
{
    i8 c;
    is >> p.x >> c >> p.y >> c >> p.z >> c >> p.w;
    return is;
}

