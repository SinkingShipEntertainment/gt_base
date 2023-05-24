

#include "gt_base/Video.h"
#include "gt_base/Image.h"
#include "gt_base/Logger.h"
#include "gt_base/string.h"

extern "C"
{
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}
#include <math.h>

bool ffmpegOutput = false;

using namespace gt;
using namespace std;

Logger l("video");  //, "./main.log");

u32 audioBitRate                      = 64000;
u32 audioSampleRate                   = 44100;
enum AVSampleFormat audioSampleFormat = AV_SAMPLE_FMT_S16;

static AVFrame * alloc_audio_frame(enum AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples)
{
  AVFrame * frame = av_frame_alloc();
  if(!frame) { throw runtime_error("alloc_audio_frame: av_frame_alloc failed"); }
  frame->format         = sample_fmt;
  frame->channel_layout = channel_layout;
  frame->sample_rate    = sample_rate;
  frame->nb_samples     = nb_samples;
  if(nb_samples)
  {
    if(av_frame_get_buffer(frame, 0) < 0) { throw runtime_error("alloc_audio_frame: av_frame_get_buffer failed"); }
  }
  return frame;
}

void endcode(AVFrame const * avFrame, AVStream * avStream, AVCodecContext * codecContext, AVFormatContext * fmtCtx)
{
  i32 ret = avcodec_send_frame(codecContext, avFrame);

  if(ret < 0) { throw runtime_error("avcodec_send_frame failed"); }

  while(ret >= 0)
  {
    AVPacket avPacket = {0};

    ret = avcodec_receive_packet(codecContext, &avPacket);
    if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) { break; }

    else if(ret < 0) { throw runtime_error("avcodec_receive_packet failed"); }

    /// this probably isn't necessary for video as the time_base's are the same, audio is a different story
    av_packet_rescale_ts(&avPacket, codecContext->time_base, avStream->time_base);
    avPacket.stream_index = avStream->index;

    // log_packet(fmtCtx, &avPacket);

    ret = av_interleaved_write_frame(fmtCtx, &avPacket);
    av_packet_unref(&avPacket);
    if(ret < 0) { throw runtime_error("av_interleaved_write_frame failed"); }
  }
}

void avLogCb(void * ptr, i32 level, char const * fmt, va_list vargs)
{
  va_list vl2;
  char line[1024];
  static int print_prefix = 1;
  va_copy(vl2, vargs);
  av_log_format_line(ptr, level, fmt, vl2, line, sizeof(line), &print_prefix);
  va_end(vl2);

  if(ffmpegOutput) l.r(line);
}

Video::Video(string const & outPath, string const & avFormatStr, i32 const fps, i64 const bitRate, i32 const gopSize, i32 const max_b_frames)
    : frameCounter(0),
      hasAudio(false),
      _written(false),
      _outPath(outPath),
      _fmtCtx(nullptr),
      _avStream(nullptr),
      _avFrame(nullptr),
      _codecContext(nullptr),
      _opts(nullptr),
      _swsContext(nullptr),
      _srcPixelFormat(AV_PIX_FMT_RGB24),
      _dstPixelFormat(AV_PIX_FMT_YUV420P),
      _swrContext(nullptr)
{
  av_log_set_callback(&avLogCb);

  bool isInterframe = true;  /// interframe or intraframe: h264 is but dnxhd for example is not

  string avFormatStr_(avFormatStr);

  if(avFormatStr_ == "h264")
  {
    avFormatStr_ = "mov";  /// mov uses h264 via lib264
  }
  else
  {
    l.e(f("unsupported format: %") % avFormatStr_);
    return;
  }

  if(avformat_alloc_output_context2(&_fmtCtx, NULL, avFormatStr_.c_str(), _outPath.c_str()) < 0)
  {
    throw runtime_error("avformat_alloc_output_context2 failed");
  }
  // l.d(f("format: %\n") % _fmtCtx->oformat->name);

  if(_fmtCtx->oformat->video_codec == AV_CODEC_ID_NONE) { throw runtime_error("avformat_alloc_output_context2 failed"); }

  _videoCodec = avcodec_find_encoder(_fmtCtx->oformat->video_codec);
  if(!_videoCodec) { throw runtime_error("avcodec_find_encoder failed"); }
  l.r(f("codec: %\n") % _videoCodec->name);

  _codecContext = avcodec_alloc_context3(_videoCodec);
  if(!_codecContext) { throw runtime_error("avcodec_alloc_context3 failed"); }

  _codecContext->codec_id = _fmtCtx->oformat->video_codec;
  _codecContext->bit_rate = bitRate;

  _codecContext->time_base.num = 1;
  _codecContext->time_base.den = fps;
  // _codecContext->framerate.num = fps;
  // _codecContext->framerate.den = 1;

  if(isInterframe)
  {
    _codecContext->gop_size     = gopSize;
    _codecContext->max_b_frames = max_b_frames;
    // codeContext->rc_min_rate
    // codeContext->rc_max_rate
  }

  _avStream = avformat_new_stream(_fmtCtx, NULL);
  if(!_avStream) { throw runtime_error("avformat_new_stream failed"); }
  _avStream->id = _fmtCtx->nb_streams - 1;

  // _avStream->time_base.num = 1;
  // _avStream->time_base.den = fps;
  _avStream->time_base = _codecContext->time_base;

  _avFrame = av_frame_alloc();
  if(!_avFrame) { throw runtime_error("av_frame_alloc failed"); }
  _avFrame->pts = 0;
  l.r("\n");

  /// codec specific options
  // av_dict_set(&_opts, "crf", "23", 0); /// 0 23 (0 - 51) 0 is loseless, overrides bit rate
}

Video::~Video()
{
  if(!this->_written) { this->write(); }
}

/// @brief add audio to the video, currently can only add one audio once per Video instance
/// NOTE: currently a work in progress
/// @param filePath
void Video::addAudio(string const & filePath) { _audioPath = filePath; }

void Video::addFrame(Image const & img)
{
  if(frameCounter == 0)  /// initialize
  {
    _codecContext->width  = img.width;
    _codecContext->height = img.height;

    if(_fmtCtx->oformat->flags & AVFMT_GLOBALHEADER) _codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;  /// TODO: move this into the constructor?

    if(img.numComps == 4) { _srcPixelFormat = AV_PIX_FMT_RGBA; }
    _codecContext->pix_fmt = _dstPixelFormat;

    if(avcodec_open2(_codecContext, _videoCodec, &_opts) < 0) { throw runtime_error("avcodec_open2 failed"); }

    // avFrame->width  = outWidth;
    // avFrame->height = outHeight;
    _avFrame->width  = _codecContext->width;
    _avFrame->height = _codecContext->height;

    _avFrame->format = _codecContext->pix_fmt;
    if(av_frame_get_buffer(_avFrame, 0) < 0) { throw runtime_error("av_frame_get_buffer failed"); }

    if(avcodec_parameters_from_context(_avStream->codecpar, _codecContext) < 0) { throw runtime_error("avcodec_parameters_from_context failed"); }

    // l.i("__ av dump format __");
    av_dump_format(_fmtCtx, 0, _outPath.c_str(), 1);  // NOTE: just a printout
    l.r("\n");


    _swsContext = sws_getContext(_codecContext->width, _codecContext->height, _srcPixelFormat, _avFrame->width, _avFrame->height,
                                 _codecContext->pix_fmt, 0, 0, 0, 0);
    if(!_swsContext) { throw runtime_error("sws_getContext failed"); }

    ///
    /// audio
    ///
    if(_audioPath.size())
    {
      if(_fmtCtx->oformat->audio_codec == AV_CODEC_ID_NONE) { throw runtime_error("Video::addFrame: format doesn't support audio"); }

      /// out audio
      AVCodec const * outAudioCodec = avcodec_find_encoder(_fmtCtx->oformat->audio_codec);
      if(!outAudioCodec) { throw runtime_error("Video::addFrame: avcodec_find_encoder failed"); }
      
      AVCodecContext * outAudioContext = avcodec_alloc_context3(outAudioCodec);
      outAudioContext->sample_fmt  = outAudioCodec->sample_fmts ? outAudioCodec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
      outAudioContext->bit_rate    = audioBitRate;
      outAudioContext->sample_rate = audioSampleRate;
      if(outAudioCodec->supported_samplerates) 
      {
        outAudioContext->sample_rate = outAudioCodec->supported_samplerates[0];
        for(i32 i = 0; outAudioCodec->supported_samplerates[i]; i++) 
        {
          if(outAudioCodec->supported_samplerates[i] == audioSampleRate)
            outAudioContext->sample_rate = audioSampleRate;
        }
      }

      AVChannelLayout outChannelLayout;
      outChannelLayout.order = AV_CHANNEL_ORDER_NATIVE;
      outChannelLayout.u.mask = AV_CH_LAYOUT_STEREO;
      outChannelLayout.nb_channels = 2;
      av_channel_layout_copy(&outAudioContext->ch_layout, &outChannelLayout);

      AVStream * outAudioStream = avformat_new_stream(_fmtCtx, nullptr);
      outAudioStream->id = _fmtCtx->nb_streams - 1;
      outAudioStream->time_base.num = 1;
      outAudioStream->time_base.den = outAudioContext->sample_rate;
    
      if(avcodec_open2(outAudioContext, outAudioCodec, nullptr) < 0) { throw runtime_error("Video::addFrame: avcodec_open2 failed"); }

      if(avcodec_parameters_from_context(outAudioStream->codecpar, outAudioContext) < 0) { throw runtime_error("Video::addFrame: avcodec_parameters_from_context failed"); }

      i32 numSamples = 0;
      if(outAudioCodec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE) { numSamples = 10000; }
      else { numSamples = outAudioContext->frame_size; }

      AVFrame * outAudioFrame = av_frame_alloc();
      outAudioFrame->format = outAudioContext->sample_fmt;
      av_channel_layout_copy(&outAudioFrame->ch_layout, &outAudioContext->ch_layout);
      outAudioFrame->sample_rate = outAudioContext->sample_rate;
      outAudioFrame->nb_samples = numSamples;
      if(numSamples)
      {
        if(av_frame_get_buffer(outAudioFrame, 0) < 0) { throw runtime_error("Video::addFrame: av_frame_get_buffer failed"); }
      }
      /// end of out audio
      
      
      /// input audio
      AVFormatContext * inAudioFmtCtx = nullptr;
      if(avformat_open_input(&inAudioFmtCtx, _audioPath.c_str(), NULL, NULL) < 0)
      {
        throw runtime_error("Video::addFrame: avformat_open_input failed");
      }
      if(avformat_find_stream_info(inAudioFmtCtx, NULL) < 0) { throw runtime_error("Video::addFrame: avformat_find_stream_info failed"); }

      AVCodec const * inAudioCodec = nullptr;
      i32 audioStreamIdx           = av_find_best_stream(inAudioFmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, &inAudioCodec, 0);
      if(audioStreamIdx < 0) { throw runtime_error("Video::addFrame: av_find_best_stream failed"); }

      AVStream * inAudioAvStream = inAudioFmtCtx->streams[audioStreamIdx];

      AVCodecContext * inAudioCodecContext = avcodec_alloc_context3(inAudioCodec);
      if(!inAudioCodecContext) { throw runtime_error("Video::addFrame: avcodec_alloc_context3 failed"); }
      avcodec_parameters_to_context(inAudioCodecContext, inAudioAvStream->codecpar);
      if(avcodec_open2(inAudioCodecContext, inAudioCodec, NULL) < 0) { throw runtime_error("Video::addFrame: avcodec_open2 failed"); }
      /// end of input audio


      if(swr_alloc_set_opts2(&_swrContext, &outAudioContext->ch_layout, outAudioContext->sample_fmt, outAudioContext->sample_rate,
                             &inAudioCodecContext->ch_layout, inAudioCodecContext->sample_fmt, inAudioCodecContext->sample_rate, 0, nullptr) < 0)

      // if(swr_alloc_set_opts2(&_swrContext, &inAudioCodecContext->ch_layout, inAudioCodecContext->sample_fmt, inAudioCodecContext->sample_rate,
      //                        &inAudioCodecContext->ch_layout, inAudioCodecContext->sample_fmt, inAudioCodecContext->sample_rate, 0, nullptr) < 0)
      {
        throw runtime_error("Video::addFrame: swr_alloc_set_opts2 failed");
      }
      if(swr_init(_swrContext) < 0) { throw runtime_error("Video::addFrame: swr_init failed"); }

      AVPacket * packet = av_packet_alloc();
      AVFrame * frame   = av_frame_alloc();
      while(true)
      {
        if(av_read_frame(inAudioFmtCtx, packet) < 0) break;

        if(packet->stream_index == audioStreamIdx)
        {
          if(avcodec_send_packet(inAudioCodecContext, packet) < 0)
          {
            throw runtime_error("Video::addFrame avcodec_send_packet failed"); 
            av_log(NULL, AV_LOG_ERROR, "Error while sending a packet to the decoder\n");
            break;
          }

          while(true)
          {
            i32 ret = avcodec_receive_frame(inAudioCodecContext, frame);
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) { break; }
            else if(ret < 0)
            {
              throw runtime_error("Video::addFrame avcodec_receive_frame failed"); 
              // av_log(NULL, AV_LOG_ERROR, "Error while receiving a frame from the decoder\n");
              // break;
            }
          }

          /// now write to the out context
          i64 delay         = swr_get_delay(_swrContext, outAudioContext->sample_rate);
          i32 dstNumSamples = av_rescale_rnd(delay + frame->nb_samples, outAudioContext->sample_rate, outAudioContext->sample_rate, AV_ROUND_UP);
          // av_assert0(dstNumSamples == frame->nb_samples);

          if(av_frame_make_writable(outAudioFrame) < 0)
          {
            throw runtime_error("Video::addFrame: av_frame_make_writable failed");
          }

          if(swr_convert(_swrContext, outAudioFrame->data, dstNumSamples, (uint8_t const **)frame->data, frame->nb_samples) < 0)
          {
            throw runtime_error("Video::addFrame: swr_convert failed"); 
          }
        }
      }
      av_packet_unref(packet);

      avcodec_free_context(&inAudioCodecContext);

      avformat_free_context(inAudioFmtCtx);
    }

    /// open the file
    if(avio_open(&_fmtCtx->pb, _outPath.c_str(), AVIO_FLAG_WRITE) < 0) { throw runtime_error("avio_open failed"); }
    if(avformat_write_header(_fmtCtx, &_opts) < 0)  /// &opt
    {
      throw runtime_error("avformat_write_header failed");
    }

  }  /// done initializing

  if(av_frame_make_writable(_avFrame) < 0) { throw runtime_error("av_frame_make_writable failed"); }

  u8 const * const * srcSlice = &img.data;
  i32 const * srcStride       = reinterpret_cast<i32 const *>(&img.pitch);
  if(sws_scale(_swsContext, srcSlice, srcStride, 0, _codecContext->height, _avFrame->data, _avFrame->linesize) < 0)
  {
    throw runtime_error("sws_scale failed");
  }

  /// av_compare_ts ?

  // _avFrame->pts = ++frameCounter;
  _avFrame->pts = frameCounter++;

  endcode(_avFrame, _avStream, _codecContext, _fmtCtx);
  // _avFrame->pts = frameCounter++;
}

void Video::write()
{
  endcode(nullptr, _avStream, _codecContext, _fmtCtx);

  /// cleanup
  av_dict_free(&_opts);

  av_write_trailer(_fmtCtx);

  avcodec_free_context(&_codecContext);
  av_frame_free(&_avFrame);

  sws_freeContext(_swsContext);

  avcodec_free_context(&_codecContext);

  swr_free(&_swrContext);

  avio_closep(&_fmtCtx->pb);
  avformat_free_context(_fmtCtx);

  this->_written = true;
}