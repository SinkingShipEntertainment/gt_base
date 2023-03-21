

#pragma once

#include "gt_base/types/base.h"

namespace gt
{

class Image;

void genNormals(Image const & inDispImg, Image & outImg);

void latlongToCubeMap(Image const & srcImg, Image & dstImg, u32 const faceWidth = 512, u32 const dstBytesPerComp = 4);

void imageIrradianceFilterSh(Image const & srcImg, Image & dstImg, u32 const faceWidth = 256, u32 dstBytesPerComp = 4);

void buildCubemapNormalSolidAngle(Image & img, bool edgeFix);

};
