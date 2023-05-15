

#include "gt_base/Video.h"
#include "gt_base/Image.h"
#include "gt_base/Logger.h"
#include "gt_base/string.h"
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
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
    : _outPath(outPath),
      frameCounter(0),
      hasAudio(false),
      _written(false),
      fmtCtx(nullptr),
      opts(nullptr),
      codecContext(nullptr),
      avStream(nullptr),
      avFrame(nullptr),
      swsContext(nullptr),
      srcFormat(AV_PIX_FMT_RGB24),
      dstFormat(AV_PIX_FMT_YUV420P),
      _audioCodecContext(nullptr),
      _audioAvStream(nullptr),
      _audioAvFrame(nullptr),
      _tmpAudioAvFrame(nullptr),
      _audioSwrContext(nullptr)
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

  if(avformat_alloc_output_context2(&fmtCtx, NULL, avFormatStr_.c_str(), _outPath.c_str()) < 0)
  {
    throw runtime_error("avformat_alloc_output_context2 failed");
  }
  l.d(f("format: %\n") % fmtCtx->oformat->name);

  if(fmtCtx->oformat->video_codec == AV_CODEC_ID_NONE) { throw runtime_error("avformat_alloc_output_context2 failed"); }

  codec = avcodec_find_encoder(fmtCtx->oformat->video_codec);
  if(!codec) { throw runtime_error("avcodec_find_encoder failed"); }
  l.r(f("codec: %\n") % codec->name);

  codecContext = avcodec_alloc_context3(codec);
  if(!codecContext) { throw runtime_error("avcodec_alloc_context3 failed"); }

  codecContext->codec_id = fmtCtx->oformat->video_codec;
  codecContext->bit_rate = bitRate;

  codecContext->time_base.num = 1;
  codecContext->time_base.den = fps;
  // codecContext->framerate.num = fps;
  // codecContext->framerate.den = 1;

  if(isInterframe)
  {
    codecContext->gop_size     = gopSize;
    codecContext->max_b_frames = max_b_frames;
    // codeContext->rc_min_rate
    // codeContext->rc_max_rate
  }

  avStream = avformat_new_stream(fmtCtx, NULL);
  if(!avStream) { throw runtime_error("avformat_new_stream failed"); }
  avStream->id        = fmtCtx->nb_streams - 1;

  // avStream->time_base.num = 1;
  // avStream->time_base.den = fps;
  avStream->time_base = codecContext->time_base;

  avFrame = av_frame_alloc();
  if(!avFrame) { throw runtime_error("av_frame_alloc failed"); }
  avFrame->pts = 0;
  l.r("\n");

  /// codec specific options
  // av_dict_set(&opts, "crf", "23", 0); /// 0 23 (0 - 51) 0 is loseless, overrides bit rate
}

Video::~Video()
{
  if(!this->_written) { this->write(); }
}

/// @brief add audio to the video, currently can only add one audio once per Video instance
/// NOTE: currently a work in progress
/// @param filePath 
void Video::addAudio(string const & filePath)
{
  if(fmtCtx->oformat->audio_codec == AV_CODEC_ID_NONE) { throw runtime_error("Video::addAudio: format doesn't support audio"); }

  _audioCodec = avcodec_find_encoder(fmtCtx->oformat->audio_codec);
  if(!_audioCodec) { throw runtime_error("Video::addAudio: avcodec_find_encoder failed"); }
  l.r(f("audio codec: %\n") % _audioCodec->name);

  _audioAvStream = avformat_new_stream(fmtCtx, NULL);
  if(!_audioAvStream) { throw runtime_error("Video::addAudio: avformat_new_stream failed"); }

  _audioAvStream->id = fmtCtx->nb_streams - 1;

  _audioCodecContext = avcodec_alloc_context3(_audioCodec);
  if(!_audioCodecContext) { throw runtime_error("Video::addAudio: avcodec_alloc_context3 failed"); }

  _audioCodecContext->sample_fmt = _audioCodec->sample_fmts ? _audioCodec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;

  _audioCodecContext->bit_rate    = audioBitRate;
  _audioCodecContext->sample_rate = audioSampleRate;
  if(_audioCodec->supported_samplerates)
  {
    _audioCodecContext->sample_rate = _audioCodec->supported_samplerates[0];
    for(u32 i = 0; _audioCodec->supported_samplerates[i]; i++)
    {
      if(_audioCodec->supported_samplerates[i] == audioSampleRate) _audioCodecContext->sample_rate = audioSampleRate;
    }
  }

  /// NOTE: av_get_channel_layout_nb_channels causes a linking error, uncomment when this is resolved
  // _audioCodecContext->channels       = av_get_channel_layout_nb_channels(_audioCodecContext->channel_layout);

  _audioCodecContext->channel_layout = AV_CH_LAYOUT_STEREO;
  if(_audioCodec->channel_layouts)
  {
    _audioCodecContext->channel_layout = _audioCodec->channel_layouts[0];
    for(u32 i = 0; _audioCodec->channel_layouts[i]; i++)
    {
      if(_audioCodec->channel_layouts[i] == AV_CH_LAYOUT_STEREO) _audioCodecContext->channel_layout = AV_CH_LAYOUT_STEREO;
    }
  }

  /// NOTE: av_get_channel_layout_nb_channels causes a linking error, uncomment when this is resolved
  // _audioCodecContext->channels = av_get_channel_layout_nb_channels(_audioCodecContext->channel_layout);

  _audioAvStream->time_base.num = 1;
  _audioAvStream->time_base.den = _audioCodecContext->sample_rate;  
  // _audioAvStream->time_base    = (AVRational){1, _audioCodecContext->sample_rate};

  if(fmtCtx->oformat->flags & AVFMT_GLOBALHEADER) _audioCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  this->hasAudio = true;

  if(avcodec_open2(_audioCodecContext, _audioCodec, &opts) < 0) { throw runtime_error("Video::addAudio: avcodec_open2 failed"); }

  /// init signal generator
  _t      = 0;
  _tincr  = 2 * M_PI * 110.0 / _audioCodecContext->sample_rate;
  _tincr2 = 2 * M_PI * 110.0 / _audioCodecContext->sample_rate / _audioCodecContext->sample_rate;  /// increment frequency by 110 Hz per second

  i32 nb_samples;

  if(_audioCodecContext->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
    nb_samples = 10000;
  else
    nb_samples = _audioCodecContext->frame_size;

  _audioAvFrame = alloc_audio_frame(_audioCodecContext->sample_fmt, _audioCodecContext->channel_layout, _audioCodecContext->sample_rate, nb_samples);
  _tmpAudioAvFrame = alloc_audio_frame(audioSampleFormat, _audioCodecContext->channel_layout, _audioCodecContext->sample_rate, nb_samples);

  if(avcodec_parameters_from_context(_audioAvStream->codecpar, _audioCodecContext) < 0) { throw runtime_error("Video::addAudio: avcodec_parameters_from_context failed"); }

  /// NOTE: swr_alloc causes a linking error, uncomment when this is resolved
  // _audioSwrContext= swr_alloc();
  if(!_audioSwrContext) { throw runtime_error("Video::addAudio: swr_alloc failed"); }

  av_opt_set_int       (_audioSwrContext, "in_channel_count",   _audioCodecContext->channels,       0);
  av_opt_set_int       (_audioSwrContext, "in_sample_rate",     _audioCodecContext->sample_rate,    0);
  av_opt_set_sample_fmt(_audioSwrContext, "in_sample_fmt",      audioSampleFormat, 0);
  av_opt_set_int       (_audioSwrContext, "out_channel_count",  _audioCodecContext->channels,       0);
  av_opt_set_int       (_audioSwrContext, "out_sample_rate",    _audioCodecContext->sample_rate,    0);
  av_opt_set_sample_fmt(_audioSwrContext, "out_sample_fmt",     _audioCodecContext->sample_fmt,     0);

  /// NOTE: swr_init causes a linking error, uncomment when this is resolved
  // if(swr_init(_audioSwrContext) < 0) {throw runtime_error("Video::addAudio: swr_init failed"); }
  
}

void Video::addFrame(Image const & img)
{
  if(frameCounter == 0)  /// initialize
  {
    codecContext->width  = img.width;
    codecContext->height = img.height;

    if(fmtCtx->oformat->flags & AVFMT_GLOBALHEADER) codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;  /// TODO: move this into the constructor?

    if(img.numComps == 4)
    {
      srcFormat = AV_PIX_FMT_RGBA;
    }
    codecContext->pix_fmt = dstFormat;

    if(avcodec_open2(codecContext, codec, &opts) < 0) { throw runtime_error("avcodec_open2 failed"); }

    // avFrame->width  = outWidth;
    // avFrame->height = outHeight;
    avFrame->width  = codecContext->width;
    avFrame->height = codecContext->height;

    avFrame->format = codecContext->pix_fmt;
    if(av_frame_get_buffer(avFrame, 0) < 0) { throw runtime_error("av_frame_get_buffer failed"); }

    if(avcodec_parameters_from_context(avStream->codecpar, codecContext) < 0) { throw runtime_error("avcodec_parameters_from_context failed"); }

    // l.i("__ av dump format __");
    av_dump_format(fmtCtx, 0, _outPath.c_str(), 1);  // NOTE: just a printout
    l.r("\n");

    if(avio_open(&fmtCtx->pb, _outPath.c_str(), AVIO_FLAG_WRITE) < 0) { throw runtime_error("avio_open failed"); }

    if(avformat_write_header(fmtCtx, &opts) < 0)  /// &opt
    {
      throw runtime_error("avformat_write_header failed");
    }

    swsContext =
        sws_getContext(codecContext->width, codecContext->height, srcFormat, avFrame->width, avFrame->height, codecContext->pix_fmt, 0, 0, 0, 0);
    if(!swsContext) { throw runtime_error("sws_getContext failed"); }

  }  /// done initializing

  if(av_frame_make_writable(avFrame) < 0) { throw runtime_error("av_frame_make_writable failed"); }

  u8 const * const * srcSlice = &img.data;
  i32 const * srcStride       = reinterpret_cast<i32 const *>(&img.pitch);
  if(sws_scale(swsContext, srcSlice, srcStride, 0, codecContext->height, avFrame->data, avFrame->linesize) < 0)
  {
    throw runtime_error("sws_scale failed");
  }
  
  /// av_compare_ts ?

  // avFrame->pts = ++frameCounter;
  avFrame->pts = frameCounter++;

  endcode(avFrame, avStream, codecContext, fmtCtx);
  // avFrame->pts = frameCounter++;
}

void Video::write()
{
  endcode(nullptr, avStream, codecContext, fmtCtx);

  /// cleanup
  av_dict_free(&opts);

  av_write_trailer(fmtCtx);

  avcodec_free_context(&codecContext);
  av_frame_free(&avFrame);
  sws_freeContext(swsContext);

  avio_closep(&fmtCtx->pb);
  avformat_free_context(fmtCtx);

  this->_written = true;
}