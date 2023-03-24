
#pragma once

#include "gt_base/types/base.h"
#include "gt_base/types/f32x4.h"
#include <OpenColorIO/OpenColorIO.h>

namespace gt
{

class f32x2;
// class f32x4;
class Image;
class Text;

/// NOTE: trying output as the first argument, followed by input arguments (so you can have default input args) 
void transformColor(Image & img, std::string const & inColorSpace, std::string const & outColorSpace);
extern OCIO_NAMESPACE::ConstConfigRcPtr ocioConfig;

};
