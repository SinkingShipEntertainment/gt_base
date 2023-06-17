
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
  /// @param filePath 
  void addAudio(std::string const & filePath, u32 numFrames);

  /// @brief 
  /// @param img 
  void addFrame(Image const & img);

  /// @brief writes the video to the file specified with the outPath argument in the constructor
  void write();

  i64 frameCounter;
  // i64 audioSampleCounter;
  bool hasAudio;
  bool isInterframe;

 private:

  
  bool _written;
  std::string _outPath;
  std::string _avFormatStr;
  i32 _fps;
  i64 _bitRate;
  i32 _gopSize;
  i32 _max_b_frames;

  std::string _audioPath;

  AVFormatContext * _fmtCtx;

  /// video
  SwsContext * _swsContext;
  AVCodecContext * _videoCodecContext;
  AVStream * _videoStream;
  AVFrame * _videoFrame;
  
  AVPixelFormat _srcPixelFormat;
  AVPixelFormat _dstPixelFormat;

  bool _bt709ColorSpace;

  /// audio
  SwrContext * _swrContext;
  AVCodecContext * _audioCodecContext;
  AVStream * _audioStream;
  std::vector<AVFrame*> _audioFrames; 

  f32 _audioDuration;
  // u32 _numAudioFrames;
  // i64 _audioFramePts;
};

}  // namespace gt
