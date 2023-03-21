
#include "gt_base/types/f32x2.h"
#include "gt_base/types/f32x3.h"
#include "gt_base/types/f32x4.h"
#include "gt_base/functions.h"
#include "gt_base/Image.h"
#include "gt_base/imgUtils3d.h"

using namespace gt;
using namespace std;

/// srcImg: assumed to be a displacement map
/// dstImg: normals in rgb, displacement in alpha
void gt::genNormals(Image const & srcImg, Image & dstImg)
{
  f32 sW = 1.f / srcImg.width;
  f32 sH = 1.f / srcImg.height;

  for(u32 y = 0; y < srcImg.height; y++)
  {
    for(u32 x = 0; x < srcImg.width; x++)
    {
      f32 htl = srcImg.getPixel(x - sW, y + sH).x;
      f32 hml = srcImg.getPixel(x - sW, y).x;
      f32 hbl = srcImg.getPixel(x - sW, y - sH).x;

      f32 htm = srcImg.getPixel(x, y + sH).x;
      f32 hmm = srcImg.getPixel(x, y).x;
      f32 hbm = srcImg.getPixel(x, y - sH).x;

      f32 htr = srcImg.getPixel(x + sW, y + sH).x;
      f32 hmr = srcImg.getPixel(x + sW, y).x;
      f32 hbr = srcImg.getPixel(x + sW, y - sH).x;

      f32 tw = (hml - hmm) + (hmm - hmr) + ((htl - hmm) + (hmm - hbr) + (hbl - hmm) + (hmm - htr)) * 0.5;

      f32 bw = (htm - hmm) + (hmm - hbm) + ((htl - hmm) + (hmm - hbr) + (htr - hmm) + (hmm - hbl)) * 0.5;

      f32x3 n(tw, bw, 1.f - abs(tw + bw) * 0.5f);

      n.y = -n.y;

      n.normalize();

      dstImg.setPixel(x, y, f32x4(n.x, n.y, n.z, hmm));
    }
  }
}

///
/// based on the cmft implementation: https://github.com/dariomanesku/cmft
///
#define CMFT_PI 3.14159265358979323846f
#define CMFT_RPI 0.31830988618379067153f

f32x3 const faceUvVectors[6][3] = {{
                                       // +x face
                                       {0.0f, 0.0f, -1.0f},  // u -> -z
                                       {0.0f, -1.0f, 0.0f},  // v -> -y
                                       {1.0f, 0.0f, 0.0f},   // +x face
                                   },
                                   {
                                       // -x face
                                       {0.0f, 0.0f, 1.0f},   // u -> +z
                                       {0.0f, -1.0f, 0.0f},  // v -> -y
                                       {-1.0f, 0.0f, 0.0f},  // -x face
                                   },
                                   {
                                       // +y face
                                       {1.0f, 0.0f, 0.0f},  // u -> +x
                                       {0.0f, 0.0f, 1.0f},  // v -> +z
                                       {0.0f, 1.0f, 0.0f},  // +y face
                                   },
                                   {
                                       // -y face
                                       {1.0f, 0.0f, 0.0f},   // u -> +x
                                       {0.0f, 0.0f, -1.0f},  // v -> -z
                                       {0.0f, -1.0f, 0.0f},  // -y face
                                   },
                                   {
                                       // +z face
                                       {1.0f, 0.0f, 0.0f},   // u -> +x
                                       {0.0f, -1.0f, 0.0f},  // v -> -y
                                       {0.0f, 0.0f, 1.0f},   // +z face
                                   },
                                   {
                                       // -z face
                                       {-1.0f, 0.0f, 0.0f},  // u -> -x
                                       {0.0f, -1.0f, 0.0f},  // v -> -y
                                       {0.0f, 0.0f, -1.0f},  // -z face
                                   }};

f32x3 texelCoordToVec(f32 u, f32 v, u8 faceId)
{
  f32x3 r;

  r.x = faceUvVectors[faceId][0].x * u + faceUvVectors[faceId][1].x * v + faceUvVectors[faceId][2].x;
  r.y = faceUvVectors[faceId][0].y * u + faceUvVectors[faceId][1].y * v + faceUvVectors[faceId][2].y;
  r.z = faceUvVectors[faceId][0].z * u + faceUvVectors[faceId][1].z * v + faceUvVectors[faceId][2].z;

  r.normalize();

  return r;
}

f32x2 latLongFromVec(f32x3 const vec)
{
  f32 const phi   = atan2f(vec.x, vec.z);
  f32 const theta = acosf(vec.y);

  f32x2 r;
  r.x = (CMFT_PI + phi) * (0.5f / CMFT_PI);
  r.y = theta * CMFT_RPI;
  return r;
}

/// srcImg is assumed to be 32bit floating point rgba
void gt::latlongToCubeMap(Image const & srcImg, Image & dstImg, u32 const faceWidth, u32 const dstBytesPerComp)
{
  u32 const & faceHeight = faceWidth;

  u32 const srcWidthMinusOne  = srcImg.width - 1;
  u32 const srcHeightMinusOne = srcImg.height - 1;

  dstImg.set(faceWidth * 6, faceHeight, 4, dstBytesPerComp, Image::FP);
  dstImg.isCubeMap = true;

  u32 const bytesPerFace = dstImg.bytesPerPixel * faceWidth * faceHeight;

  f32 const invFaceWidth  = 1.0f / static_cast<f32>(faceWidth);
  f32 const invFaceHeight = 1.0f / static_cast<f32>(faceHeight);

  for(u8 faceId = 0; faceId < 6; ++faceId)
  {
    u8 * faceData = dstImg.data + faceId * bytesPerFace;

    for(u32 y = 0; y < faceHeight; ++y)
    {
      for(u32 x = 0; x < faceWidth; ++x)
      {
        // cubemap (u,v) on current face in [-1 - 1] space
        f32 const u = 2.0f * x * invFaceWidth - 1.0f;
        f32 const v = 2.0f * y * invFaceHeight - 1.0f;

        f32x3 const vec = texelCoordToVec(u, v, faceId);

        // convert cubemap vector (x,y,z) to latlong (u,v) [0 - 1] space
        f32x2 const latlong = latLongFromVec(vec);

        // convert from [0..1] to [0..(size-1)] range
        f32x2 const latlongTs(latlong.x * srcWidthMinusOne, latlong.y * srcHeightMinusOne);

        ///
        /// bilinear interpolation
        ///
        u32 const x0 = latlongTs.x;
        u32 const y0 = latlongTs.y;
        u32 const x1 = min(x0 + 1, srcWidthMinusOne);
        u32 const y1 = min(y0 + 1, srcHeightMinusOne);

        f32 const tx    = latlongTs.x - x0;  /// just the decimal
        f32 const ty    = latlongTs.y - y0;
        f32 const invTx = 1.0f - tx;
        f32 const invTy = 1.0f - ty;

        f32x4 p0 = srcImg.getPixel(x0, y0) * (invTx * invTy);
        f32x4 p1 = srcImg.getPixel(x1, y0) * (tx * invTy);
        f32x4 p2 = srcImg.getPixel(x0, y1) * (invTx * ty);
        f32x4 p3 = srcImg.getPixel(x1, y1) * (tx * ty);

        f32 const r = p0.x + p1.x + p2.x + p3.x;
        f32 const g = p0.y + p1.y + p2.y + p3.y;
        f32 const b = p0.z + p1.z + p2.z + p3.z;
        f32 const a = p0.w + p1.w + p2.w + p3.w;

        f32x4 pixel(r, g, b, a);

        dstImg.setPixel(faceId * faceWidth + x, y, pixel);
      }
    }
  }
}

///
/// irradiance
///

// #define PI 3.1415926535897932384626433832795028841971693993751058
#define PI4 12.566370614359172953850573533118011536788677597500423
#define PI16 50.265482457436691815402294132472046147154710390001693
#define PI64 201.06192982974676726160917652988818458861884156000677
#define SQRT_PI 1.7724538509055160272981674833411451827975494561223871
#define SH_COEFF_NUM 25

/// http://www.rorydriscoll.com/2012/01/15/cubemap-texel-solid-angle/
f32 areaElement(f32 x, f32 y) { return atan2f(x * y, sqrtf(x * x + y * y + 1.0f)); }

/// u and v should be center adressing and in [-1.0+invSize..1.0-invSize] range.
f32 texelSolidAngle(f32 u, f32 v, f32 invFaceSize)
{
  // Specify texel area.
  f32 const x0 = u - invFaceSize;
  f32 const x1 = u + invFaceSize;
  f32 const y0 = v - invFaceSize;
  f32 const y1 = v + invFaceSize;

  // Compute solid angle of texel area.
  f32 const solidAngle = areaElement(x1, y1) - areaElement(x0, y1) - areaElement(x1, y0) + areaElement(x0, y0);

  return solidAngle;
}

/// u and v should be center adressing and in [-1.0+invSize..1.0-invSize] range.
f32x3 texelCoordToVecWarp(f32 u, f32 v, u8 faceId, f32 warpFixup)
{
  u = (warpFixup * u * u * u) + u;
  v = (warpFixup * v * v * v) + v;

  return texelCoordToVec(u, v, faceId);
}

f32 warpFixupFactor(f32 _faceSize)
{
  // Edge fixup.
  // Based on Nvtt : http://code.google.com/p/nvidia-texture-tools/source/browse/trunk/src/nvtt/CubeSurface.cpp
  if(_faceSize == 1.0f) { return 1.0f; }

  f32 const fs   = _faceSize;
  f32 const fsmo = fs - 1.0f;
  return (fs * fs) / (fsmo * fsmo * fsmo);
}

/// Creates a cubemap containing tap vectors and solid angle of each texel on the cubemap.
/// Output consists of 6 faces of specified size containing (x,y,z,angle) floats for each texel.

void gt::buildCubemapNormalSolidAngle(Image & img, bool edgeFix)
{
  u32 faceSize = img.height;

  f32 const cfs    = faceSize;
  f32 const invCfs = 1.0f / cfs;

  if(!edgeFix)
  {
    for(u8 faceId = 0; faceId < 6; ++faceId)
    {
      f32 yf = 1.0f;
      for(u32 y = 0; y < faceSize; ++y, yf += 2.0f)
      {
        f32 xf = 1.0f;
        for(u32 x = 0; x < faceSize; ++x, xf += 2.0f)
        {
          // From [0..size-1] to [-1.0+invSize .. 1.0-invSize].
          // Ref: u = 2.0*(xf+0.5)/faceSize - 1.0;
          //      v = 2.0*(yf+0.5)/faceSize - 1.0;
          f32 const u = xf * invCfs - 1.0f;
          f32 const v = yf * invCfs - 1.0f;

          f32x3 vec = texelCoordToVec(u, v, faceId);
          f32 angle = texelSolidAngle(u, v, invCfs);
          f32x4 pixel(vec, angle);
          img.setPixel(faceId * faceSize + x, y, pixel);
        }
      }
    }
  }
  else  // if (EdgeFixup::Warp == fixup)#
  {
    f32 const warp = warpFixupFactor(cfs);

    // f32 * dstPtr = (f32 *)img.data;
    for(u8 faceId = 0; faceId < 6; ++faceId)
    {
      f32 yf = 1.0f;
      for(u32 y = 0; y < faceSize; ++y, yf += 2.0f)
      {
        f32 xf = 1.0f;
        for(u32 x = 0; x < faceSize; ++x, xf += 2.0f)
        {
          f32 const u = xf * invCfs - 1.0f;
          f32 const v = yf * invCfs - 1.0f;

          f32x3 vec = texelCoordToVecWarp(u, v, faceId, warp);
          f32 angle = texelSolidAngle(u, v, invCfs);
          f32x4 pixel(vec, angle);
          img.setPixel(faceId * faceSize + x, y, pixel);
        }
      }
    }
  }
}

void evalSHBasis5(f64 * shBasis, f32x3 const & dir)
{
  f64 const x = f64(dir.x);
  f64 const y = f64(dir.y);
  f64 const z = f64(dir.z);

  f64 const x2 = x * x;
  f64 const y2 = y * y;
  f64 const z2 = z * z;

  f64 const z3 = pow(z, 3.0);

  f64 const x4 = pow(x, 4.0);
  f64 const y4 = pow(y, 4.0);
  f64 const z4 = pow(z, 4.0);

  // equations based on data from: http://ppsloan.org/publications/StupidSH36.pdf
  shBasis[0] = 1.0 / (2.0 * SQRT_PI);

  shBasis[1] = -sqrt(3.0 / PI4) * y;
  shBasis[2] = sqrt(3.0 / PI4) * z;
  shBasis[3] = -sqrt(3.0 / PI4) * x;

  shBasis[4] = sqrt(15.0 / PI4) * y * x;
  shBasis[5] = -sqrt(15.0 / PI4) * y * z;
  shBasis[6] = sqrt(5.0 / PI16) * (3.0 * z2 - 1.0);
  shBasis[7] = -sqrt(15.0 / PI4) * x * z;
  shBasis[8] = sqrt(15.0 / PI16) * (x2 - y2);

  shBasis[9]  = -sqrt(70.0 / PI64) * y * (3 * x2 - y2);
  shBasis[10] = sqrt(105.0 / PI4) * y * x * z;
  shBasis[11] = -sqrt(21.0 / PI16) * y * (-1.0 + 5.0 * z2);
  shBasis[12] = sqrt(7.0 / PI16) * (5.0 * z3 - 3.0 * z);
  shBasis[13] = -sqrt(42.0 / PI64) * x * (-1.0 + 5.0 * z2);
  shBasis[14] = sqrt(105.0 / PI16) * (x2 - y2) * z;
  shBasis[15] = -sqrt(70.0 / PI64) * x * (x2 - 3.0 * y2);

  shBasis[16] = 3.0 * sqrt(35.0 / PI16) * x * y * (x2 - y2);
  shBasis[17] = -3.0 * sqrt(70.0 / PI64) * y * z * (3.0 * x2 - y2);
  shBasis[18] = 3.0 * sqrt(5.0 / PI16) * y * x * (-1.0 + 7.0 * z2);
  shBasis[19] = -3.0 * sqrt(10.0 / PI64) * y * z * (-3.0 + 7.0 * z2);
  shBasis[20] = (105.0 * z4 - 90.0 * z2 + 9.0) / (16.0 * SQRT_PI);
  shBasis[21] = -3.0 * sqrt(10.0 / PI64) * x * z * (-3.0 + 7.0 * z2);
  shBasis[22] = 3.0 * sqrt(5.0 / PI64) * (x2 - y2) * (-1.0 + 7.0 * z2);
  shBasis[23] = -3.0 * sqrt(70.0 / PI64) * x * z * (x2 - 3.0 * y2);
  shBasis[24] = 3.0 * sqrt(35.0 / (4.0 * PI64)) * (x4 - 6.0 * y2 * x2 + y4);
}

void cubemapShCoeffs(Image const & srcImg, f64 shCoeffs[SH_COEFF_NUM][3])
{
  memset(shCoeffs, 0, SH_COEFF_NUM * 3 * sizeof(f64));

  u32 faceSize = srcImg.height;

  u32 srcBytesPerFace = (srcImg.width / 6) * srcImg.height * srcImg.bytesPerPixel;

  Image normalAngleCubeMap(srcImg.width, srcImg.height, 4, 4, Image::FP);
  buildCubemapNormalSolidAngle(normalAngleCubeMap, false);

  u32 const vectorPitch    = faceSize * normalAngleCubeMap.bytesPerPixel;
  u32 const naBytesPerFace = vectorPitch * faceSize;

  f64 weightAccum = 0.0;

  // Evaluate spherical harmonics coefficients.
  for(u8 faceId = 0; faceId < 6; ++faceId)
  {
    f32 const * srcPtr = (f32 const *)(srcImg.data + srcBytesPerFace * faceId);
    f32 const * vecPtr = (f32 const *)(normalAngleCubeMap.data + naBytesPerFace * faceId);

    for(u32 texel = 0, count = faceSize * faceSize; texel < count; ++texel, srcPtr += 4, vecPtr += 4)
    {
      f64 shBasis[SH_COEFF_NUM];
      f32x3 vec(vecPtr[0], vecPtr[1], vecPtr[2]);
      evalSHBasis5(shBasis, vec);

      f64 const r     = f64(srcPtr[0]);
      f64 const g     = f64(srcPtr[1]);
      f64 const b     = f64(srcPtr[2]);
      f64 const weight = (f64)vecPtr[3];

      for(u8 ii = 0; ii < SH_COEFF_NUM; ++ii)
      {
        shCoeffs[ii][0] += r * shBasis[ii] * weight;
        shCoeffs[ii][1] += g * shBasis[ii] * weight;
        shCoeffs[ii][2] += b * shBasis[ii] * weight;
      }

      weightAccum += weight;
    }
  }

  // Normalization: This is not really necesarry because usually PI*4 - weightAccum ~= 0.000003
  // so it doesn't change almost anything, but it doesn't cost much be more correct.
  f64 const norm = PI4 / weightAccum;
  for(u8 ii = 0; ii < SH_COEFF_NUM; ++ii)
  {
    shCoeffs[ii][0] *= norm;
    shCoeffs[ii][1] *= norm;
    shCoeffs[ii][2] *= norm;
  }
}

/// called imageIrradianceFilterSh in cmft
void gt::imageIrradianceFilterSh(Image const & srcImg, Image & dstImg, u32 const dstFaceSize, u32 dstBytesPerComp)
{
  if(!srcImg.isCubeMap)
  {
    throw runtime_error("imgUtils::imageIrradianceFilterSh: src image not a cubemap");
    // return;
  }

  f64 shRgb[SH_COEFF_NUM][3];
  cubemapShCoeffs(srcImg, shRgb);

  dstImg.set(dstFaceSize * 6, dstFaceSize, 4, dstBytesPerComp, Image::FP);

  Image normalAngleCubeMap(dstImg.width, dstImg.height, 4, 4, Image::FP);
  normalAngleCubeMap.isCubeMap = true;
  buildCubemapNormalSolidAngle(normalAngleCubeMap, false);

  u32 const vectorPitch        = dstFaceSize * normalAngleCubeMap.bytesPerPixel;
  u32 const vectorFaceDataSize = vectorPitch * dstFaceSize;

  u32 dstBytesPerFace = (dstImg.width / 6) * dstImg.height * dstImg.bytesPerPixel;

  for(u8 faceId = 0; faceId < 6; ++faceId)
  {
    f32 * dstPtr = (f32 *)(dstImg.data + dstBytesPerFace * faceId);
    f32 const * vecPtr = (f32 const *)(normalAngleCubeMap.data + vectorFaceDataSize * faceId);

    for(uint32_t texel = 0, count = dstFaceSize * dstFaceSize; texel < count; ++texel, dstPtr += 4, vecPtr += 4)
    {
      f64 shBasis[SH_COEFF_NUM];
      f32x3 vec(vecPtr[0], vecPtr[1], vecPtr[2]);
      evalSHBasis5(shBasis, vec);

      f64 rgb[3] = {0.0, 0.0, 0.0};

      // Band 0 (factor 1.0)
      rgb[0] += shRgb[0][0] * shBasis[0] * 1.0f;
      rgb[1] += shRgb[0][1] * shBasis[0] * 1.0f;
      rgb[2] += shRgb[0][2] * shBasis[0] * 1.0f;

      // Band 1 (factor 2/3).
      uint8_t ii = 1;
      for(; ii < 4; ++ii)
      {
        rgb[0] += shRgb[ii][0] * shBasis[ii] * (2.0f / 3.0f);
        rgb[1] += shRgb[ii][1] * shBasis[ii] * (2.0f / 3.0f);
        rgb[2] += shRgb[ii][2] * shBasis[ii] * (2.0f / 3.0f);
      }

      // Band 2 (factor 1/4).
      for(; ii < 9; ++ii)
      {
        rgb[0] += shRgb[ii][0] * shBasis[ii] * (1.0f / 4.0f);
        rgb[1] += shRgb[ii][1] * shBasis[ii] * (1.0f / 4.0f);
        rgb[2] += shRgb[ii][2] * shBasis[ii] * (1.0f / 4.0f);
      }

      // Band 3 (factor 0).
      ii = 16;

      // Band 4 (factor -1/24).
      for(; ii < 25; ++ii)
      {
        rgb[0] += shRgb[ii][0] * shBasis[ii] * (-1.0f / 24.0f);
        rgb[1] += shRgb[ii][1] * shBasis[ii] * (-1.0f / 24.0f);
        rgb[2] += shRgb[ii][2] * shBasis[ii] * (-1.0f / 24.0f);
      }

      dstPtr[0] = float(rgb[0]);
      dstPtr[1] = float(rgb[1]);
      dstPtr[2] = float(rgb[2]);
      dstPtr[3] = 1.0f;
    }
  }
}
