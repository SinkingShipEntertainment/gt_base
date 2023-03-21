
#pragma once

#include "gt_base/types/base.h"

namespace gt
{

class u8x3
{
public:
	
	u8x3(u8 s = 0);

	u8x3(u8 x, u8 y, u8 z);
	
	u8 x, y, z;
};

/// io
void read(std::istream & is, u8x3 & p);

std::ostream & operator << (std::ostream & os, const u8x3 & p);
std::istream & operator >> (std::istream & is, u8x3 & p);

} // namespace gt

