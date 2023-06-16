
#include "gt_base/imgUtils.h"
#include "gt_base/types/f32x2.h"
#include "gt_base/types/f32x3.h"
#include "gt_base/types/f32x4.h"
#include "gt_base/types/f32x3x3.h"
#include "gt_base/functions.h"
#include "gt_base/string.h"
#include "gt_base/Image.h"
#include <OpenColorIO/OpenColorTransforms.h>
#if defined(__GNUC__) && __GNUC__ < 8
  #include <experimental/filesystem>
  namespace filesystem = std::experimental::filesystem;
#else
  #include <filesystem>
#endif

using namespace gt;
using namespace std;

namespace OCIO = OCIO_NAMESPACE;
OCIO::ConstConfigRcPtr gt::ocioConfig = nullptr;

void gt::transformColor(Image & img, string const & inColorSpace, string const & outColorSpace)
{
  if(!ocioConfig)
  {
    i8 * ocioVal = getenv("OCIO");
    if(!ocioVal)
    {
      throw runtime_error("imgUtils::transformColor: OCIO env var not set");
    }
    
    if(!filesystem::exists(ocioVal)) throw runtime_error(f("imgUtils::transformColor: % doesn't exist") % ocioVal);

    try
    {
      ocioConfig = OCIO::GetCurrentConfig();
    }
    catch(OCIO::Exception & exception)
    {
      throw runtime_error(f("imgUtils::transformColor: OpenColorIO error caught...\n%") % exception.what());
    }
  }

  // l.r(f("OCIO version %\n") % OCIO::GetVersion());

  try
  {
    OCIO::ConstProcessorRcPtr processor = ocioConfig->getProcessor(inColorSpace.c_str(), outColorSpace.c_str());  

    OCIO::BitDepth ocioBitDepth = OCIO::BitDepth::BIT_DEPTH_UINT8;
    if(img.type == Image::FP)
    {
      if(img.bytesPerComp == 2) ocioBitDepth = OCIO::BitDepth::BIT_DEPTH_F16;
      else ocioBitDepth = OCIO::BitDepth::BIT_DEPTH_F32;
    }

    OCIO::ConstCPUProcessorRcPtr ocioPrc = processor->getOptimizedCPUProcessor(ocioBitDepth, ocioBitDepth, OCIO::OptimizationFlags::OPTIMIZATION_VERY_GOOD);

    ptrdiff_t chanStrideBytes = img.bytesPerComp;
    ptrdiff_t xStrideBytes = img.bytesPerPixel;
    ptrdiff_t yStrideBytes = img.width * img.bytesPerPixel;

    // l.i("OCIO conversion from: %, to: %" % inColorSpace % outColorSpace);

    OCIO::PackedImageDesc ocioImg(img.data, img.width, img.height, img.numComps, ocioBitDepth, chanStrideBytes, xStrideBytes, yStrideBytes);
    ocioPrc->apply(ocioImg);
  }

  catch(OCIO::Exception & exception)
  {
    throw runtime_error(f("imgUtils::transformColor: OpenColorIO error caught...\n%") % exception.what());
  }  
}


void gt::transformColor(Image & img, f32x3x3 const & mat)
{
  for(u32 y = 0; y < img.height; ++y)
  {
    for(u32 x = 0; x < img.width; ++x)
    {
      f32x4 px = img.getPixel(x, y);
      f32x3 px3 = px.xyz();
      // px3 = mat * px3;
      px3 = px3 * mat;
      img.setPixel(x, y, f32x4(px3, px.w));
    }
  }

  // if(img.type != Image::FP) throw runtime_error("imgUtils::transformColor: only FP images supported");

  // ptrdiff_t chanStrideBytes = img.bytesPerComp;
  // ptrdiff_t xStrideBytes = img.bytesPerPixel;
  // ptrdiff_t yStrideBytes = img.width * img.bytesPerPixel;

  // OCIO::PackedImageDesc ocioImg(img.data, img.width, img.height, img.numComps, OCIO::BitDepth::BIT_DEPTH_F32, chanStrideBytes, xStrideBytes, yStrideBytes);

  // OCIO::ConstCPUProcessorRcPtr ocioPrc = OCIO::MatrixTransform::Create()->createEditableCopy();
  // OCIO::MatrixTransform * matPrc = dynamic_cast<OCIO::MatrixTransform *>(ocioPrc.get());
  // matPrc->setMatrix(mat.data());

  // ocioPrc->apply(ocioImg);
}
