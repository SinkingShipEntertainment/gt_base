
#pragma once

#include "gt_base/types.h"
#include "gt_base/Image.h"
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}
#include <string>

class SwrContext;

namespace gt
{

class Image;

class Video
{
 public:
  /// @brief constructor
  /// @param outPath 
  /// @param avFormatStr 
  /// @param fps 
  /// @param bitRate assumed to be in bytes
  /// @param gopSize 
  /// @param max_b_frames 
  Video(std::string const & outPath, std::string const & avFormatStr, i32 const fps, i64 bitRate, i32 gopSize, i32 max_b_frames);

  virtual ~Video();

  /// @brief add audio to the video, currently can only add one audio once per Video instance
  /// NOTE: currently a work in progress
  /// @param filePath 
  void addAudio(std::string const & filePath);

  /// @brief 
  /// @param img 
  void addFrame(Image const & img);

  /// @brief writes the video to the file specified with the outPath argument in the constructor
  void write();

  i64 frameCounter;
  // int64_t frameCounter;

  bool hasAudio;

 private:
  
  bool _written;
  std::string _outPath;

  AVFormatContext * fmtCtx;

  AVDictionary * opts;

  /// video
  AVCodec const * codec;
  AVCodecContext * codecContext;
  AVStream * avStream;
  AVFrame * avFrame;
  SwsContext * swsContext;
  AVPixelFormat srcFormat;
  AVPixelFormat dstFormat;

  /// audio
  AVCodec const * _audioCodec;
  AVCodecContext * _audioCodecContext;
  AVStream * _audioAvStream;
  AVFrame * _audioAvFrame;
  AVFrame * _tmpAudioAvFrame;
  SwrContext * _audioSwrContext;
  f32 _t, _tincr, _tincr2;

  
};

}  // namespace gt
