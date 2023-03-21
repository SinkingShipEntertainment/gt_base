
#pragma once

#include "gt_base/types/f32x3.h"
#include <math.h>

namespace gt
{

class f32x2;
class f32x3;

class f32x4
{
public:
	
	f32x4(f32 s = 0);

	f32x4(f32 x, f32 y, f32 z, f32 w);

	f32x4(f32x2 const & p1, f32x2 const & p2);
	
	f32x4(f32x2 const & v, f32 z, f32 w);
	
	f32x4(f32x3 const & v, f32 w);

	f32x2 const xy()  const;
	f32x3 const xyz() const;

	f32 length(f32x4 const & v) const;
	
	void normalize();
    
	f32x4 const normalized() const;

	bool hasInside(f32x2 const & p) const;
	
	f32 * memAddr() {return &x;}
	
	f32 x, y, z, w;
};

f32x4 const operator + (f32x4 const & v1, f32x4 const & v2);
f32x4 const operator + (f32x4 const & v1, f32x3 const & v2);

f32x4 const operator - (f32x4 const & v1, f32x4 const & v2);
f32x4 const operator - (f32 v1, f32x4 const & v2);
f32x4 const operator - (f32x4 const & v);

f32x4 const operator * (f32x4 const & v1, f32 const & v2);

f32x2 const operator * (f32x2 const & v1, f32x4 const & v2);
f32x3 const operator * (f32x3 const & v1, f32x4 const & v2);
f32x4 const operator * (f32x4 const & v1, f32x4 const & v2);


f32x4 const operator / (f32x4 const & v1, f32x4 const & v2);

void operator += (f32x4 & v1, f32x4 const & v2);

void operator -= (f32x4 & v1, f32x4 const & v2);

void operator *= (f32x4 & v1, f32x4 const & v2);
void operator *= (f32x4 & v, f32 s);

void operator /= (f32x4 & v1, f32x4 const & v2);

void operator /= (f32x4 & v, f32 s);

bool operator == (f32x4 const & v1, f32x4 const & v2);

f32 dot(f32x4 const & v1, f32x4 const & v2);

/// io
void write(std::ostream & os, f32x4 const & v);
void read(std::istream & is, f32x4 & p);

std::ostream & operator << (std::ostream & os, f32x4 const & p);
std::istream & operator >> (std::istream & is, f32x4 & p);

} // namespace gt

