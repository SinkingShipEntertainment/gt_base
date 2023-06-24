

#include "gt_base/types.h"
#include "gt_base/types/f32x2.h"
#include "gt_base/functions.h"
#include "gt_base/string.h"
#include "gt_base/Logger.h"
#include "gt_base/Timer.h"
#include "gt_base/globals.h"
#include "gt_base/filesystem.h"
#include "gt_base/Image.h"
#include "gt_base/Text.h"
#include "gt_base/imgUtils.h"

#if defined(USE_IMG_RESAMPLER)
  #include "Base/vnImageFormat.h"
  #include "Base/vnImage.h"
  #include "Utilities/vnImageSampler.h"
  #include "Operators/vnImageResize.h"
#endif

#include <filesystem>
#include <map>

using namespace gt;
using namespace std;

Logger gt::l("test");

string imgSrcPath = "C:/Users/tom/work_offline/assets/testing/scalingTests/seqTest.0042.exr";
vector<string> scaleAlgorithms = {"nearest", "average", "bilinear", "bicubic", "catmul", "mitchell", "spline", "bspline", "lanczos", "lanczos5", "gaussian"};

u32 outWidth = 500;
u32 outHeight = 500;


i32 main(i32 argc, char * argv[])
{
  try
  {
    /// TMP
    Image img(imgSrcPath, {}, {"R", "G", "B", "A"});

    for(auto const & scaleAlgorithm: scaleAlgorithms)
    {
      Image outImg;

    #if defined(USE_IMG_RESAMPLER) /// using open source library https://github.com/ramenhut/image-resampler 
        
      CVImage cvInImg;
      vnCreateImage(VN_IMAGE_FORMAT_R32G32B32A32, img.width, img.height, &cvInImg);
      memcpy(cvInImg.QueryData(), img.data, img.bytesTotal);

      VN_IMAGE_KERNEL_TYPE scaleAlgo;
      if(scaleAlgorithm == "nearest") scaleAlgo = VN_IMAGE_KERNEL_NEAREST;
      else if(scaleAlgorithm == "average") scaleAlgo = VN_IMAGE_KERNEL_AVERAGE;
      else if(scaleAlgorithm == "bilinear") scaleAlgo = VN_IMAGE_KERNEL_BILINEAR;
      else if(scaleAlgorithm == "bicubic") scaleAlgo = VN_IMAGE_KERNEL_BICUBIC;
      else if(scaleAlgorithm == "catmul") scaleAlgo = VN_IMAGE_KERNEL_CATMULL;
      else if(scaleAlgorithm == "mitchell") scaleAlgo = VN_IMAGE_KERNEL_MITCHELL;
      else if(scaleAlgorithm == "spline") scaleAlgo = VN_IMAGE_KERNEL_SPLINE;
      else if(scaleAlgorithm == "bspline") scaleAlgo = VN_IMAGE_KERNEL_BSPLINE;
      else if(scaleAlgorithm == "lanczos") scaleAlgo = VN_IMAGE_KERNEL_LANCZOS;
      else if(scaleAlgorithm == "lanczos5") scaleAlgo = VN_IMAGE_KERNEL_LANCZOS5;
      else if(scaleAlgorithm == "gaussian") scaleAlgo = VN_IMAGE_KERNEL_GAUSSIAN;
      // else if(scaleAlgorithm == 
      else
      {
        l.w(f("unknown scale algorithm %, using bilinear") % scaleAlgorithm);
        scaleAlgo = VN_IMAGE_KERNEL_BILINEAR;
      }

      CVImage cvOutImg; 
      vnResizeImage(cvInImg, scaleAlgo, outWidth, outHeight, 0, &cvOutImg);

      outImg.set(outWidth, outHeight, img.numComps, img.bytesPerComp, img.type);
      memcpy(outImg.data, cvOutImg.QueryData(), outImg.bytesTotal);

      vnDestroyImage(&cvInImg);
      vnDestroyImage(&cvOutImg);

    #else

      outImg = img;

      u8 scaleAlgo = 0;
      if(scaleAlgorithm == "nearest") scaleAlgo = Image::NEAREST;
      else if(scaleAlgorithm == "bilinear") scaleAlgo = Image::BILINEAR;
      else if(scaleAlgorithm == "bicubic") scaleAlgo = Image::BICUBIC;
      outImg.setSize(outWidth, outHeight, true, scaleAlgo);

    #endif
    
      f32x2 startPos(20, 20);
      f32x4 bgColor(0.3, 0.3, 0.3, 0.3);
      Text text(24, "arial", f32x3(1.f, 1.f, 1.f), f("using %") % scaleAlgorithm);
      text.overlay(outImg, startPos, true, bgColor, 5.f);

      outImg.write(f("C:/Users/tom/work_offline/assets/testing/scalingTests/seqTest.0042_%.exr") % scaleAlgorithm);
    }

    return 0;

    // ///
    // /// conversion of types
    // ///
    // f32 const f32Num = 0.76f;
    // f16 const f16Num = static_cast<f16>(f32Num);
    // u8 const u8Num = f32Num * MAX_U8;
    // u16 const u16Num = f32Num * MAX_U16;
    // u32 const u32Num = f32Num * MAX_U32;

    // l.i(f("f16Num %, f32Num %, u8Num %, u16Num %, u32Num %") % f16Num % f32Num % (u16)u8Num % u16Num % u32Num);

    // f16 u8ToF16;
    // convert(u8Num, u8ToF16);
    // l.i(f("%") % u8ToF16);

    // f32 u8ToF32;
    // convert(u8Num, u8ToF32);
    // l.i(f("%") % u8ToF32);

    // u8 u16Tou8;
    // convert(u16Num, u16Tou8);
    // l.i(f("%") % (u16)u16Tou8);

    // f32 u16ToF32;
    // convert(u16Num, u16ToF32);
    // l.i(f("%") % u16ToF32);

    // f16 u16ToF16;
    // convert(u16Num, u16ToF16);
    // l.i(f("%") % u16ToF16);

    // u8 f16ToU8;
    // convert(f16Num, f16ToU8);
    // l.i(f("%") % (u16)f16ToU8);

    // u8 f32ToU8;
    // convert(f32Num, f32ToU8);
    // l.i(f("%") % (u16)f32ToU8);


    // ///
    // /// Image and text generation
    // ///
    // Timer timer;

    // Image img(1920, 1080, 3, 1, Image::UINT);

    // img.setToColor(0, 1, 1);

    // f32x2 startPos(40, 200);

    // f32x4 bgColor(0.3, 0.3, 0.3, 0.8);
    
    // Text text(64, "arial", f32x3(1.f, 0.5f, 0.5f), "Testing 123 ABC\nI like stuff blah blah blah");

    // text.overlay(img, startPos, false, bgColor, 20.f);

    // img.write("./test.exr");

    // l.i(f("done in %s") % timer.elapsed());

  }
  catch(std::exception const & e)
  {
    l.e(f("exception caught...\n%") % e.what());
  }
  catch(...)
  {
    l.e("unknown exception caught");
  }

  return 0;
}
