

#include "gt_base/Video.h"
#include "gt_base/Image.h"
#include "gt_base/Logger.h"
#include "gt_base/string.h"

extern "C"
{
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}
#include <math.h>

bool ffmpegOutput = false;

using namespace gt;
using namespace std;

Logger l("video");  //, "./main.log");

#define SYNTH_AUDIO_TEST
u32 audioBitRate                      = 64000;
u32 audioSampleRate                   = 44100;
enum AVSampleFormat audioSampleFormat = AV_SAMPLE_FMT_S16;
i64 STREAM_DURATION                   = 24;

void encode(AVFormatContext * fmtCtx, AVFrame const * avFrame, AVStream * avStream, AVCodecContext * codecContext)
{
  if(avcodec_send_frame(codecContext, avFrame) < 0) { throw runtime_error("avcodec_send_frame failed"); }

  while(true)
  {
    AVPacket avPacket = {0};

    i32 ret = avcodec_receive_packet(codecContext, &avPacket);
    if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) { break; }

    if(ret < 0) { throw runtime_error("encode: avcodec_receive_packet failed"); }

    av_packet_rescale_ts(&avPacket, codecContext->time_base, avStream->time_base);
    avPacket.stream_index = avStream->index;

    // log_packet(fmtCtx, &avPacket);

    ret = av_interleaved_write_frame(fmtCtx, &avPacket);
    av_packet_unref(&avPacket);
    if(ret < 0) { throw runtime_error("av_interleaved_write_frame failed"); }
  }
}

void decode(AVCodecContext * codecContext, vector<AVFrame *> & frames, AVPacket const * avPacket)
{
  if(avcodec_send_packet(codecContext, avPacket) < 0) { throw runtime_error("Video::decode avcodec_send_packet failed"); }

  // AVFrame * inFrame = av_frame_alloc();

  while(true)
  {
    AVFrame * inFrame = av_frame_alloc();

    i32 ret = avcodec_receive_frame(codecContext, inFrame);
    if(ret < 0)
    {
      if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
      {
        av_frame_free(&inFrame);
        // frames.emplace_back(inFrame);
        return;
      }
      else { throw runtime_error("Video::decode avcodec_receive_frame failed"); }
    }
    frames.emplace_back(inFrame);
  }
  // frames.emplace_back(inFrame);
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
      // audioSampleCounter(0),
      hasAudio(false),
      isInterframe(true),  /// interframe or intraframe: h264 is but dnxhd for example is not
      _written(false),
      _outPath(outPath),
      _fps(fps),
      _bitRate(bitRate),
      _gopSize(gopSize),
      _max_b_frames(max_b_frames),
      _fmtCtx(nullptr),
      _opts(nullptr),
      _swsContext(nullptr),
      _videoCodecContext(nullptr),
      _videoStream(nullptr),
      _videoFrame(nullptr),
      _srcPixelFormat(AV_PIX_FMT_RGB24),
      _dstPixelFormat(AV_PIX_FMT_YUV420P),
      _swrContext(nullptr),
      _audioCodecContext(nullptr),
      _audioStream(nullptr),
      _numAudioFrames(0)
// _audioFrame(nullptr)
// _audioFramePts(0)
{
  av_log_set_callback(&avLogCb);

  string avFormatStr_(avFormatStr);

  if(avFormatStr_ == "h264")
  {
    avFormatStr_ = "mov";  /// mov uses h264 via lib264
  }
  else if(avFormatStr_ == "dnxhd")  /// dnxhd is intraframe
  {
    isInterframe = false;
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

  /// codec specific options
  // av_dict_set(&_opts, "crf", "23", 0); /// 0 23 (0 - 51) 0 is loseless, overrides bit rate
}

Video::~Video()
{
  if(!this->_written) { this->write(); }
}

/// @brief add audio to the video, currently can only add one audio once per Video instance

/// @param filePath
void Video::addAudio(string const & filePath, u32 numFrames)
{
  _audioPath = filePath; /// TODO: validate audio file path
  _numAudioFrames = numFrames;
  hasAudio = true;
}

AVFrame * genAudioFrame(i32 format, AVChannelLayout const & outChannelLayout, i32 sampleRate, i32 numSamples)
{
  AVFrame * audioFrame = av_frame_alloc();
  if(!audioFrame) { throw runtime_error("Video::genAudioFrame: av_frame_alloc failed"); }
  audioFrame->format = format;
  av_channel_layout_copy(&audioFrame->ch_layout, &outChannelLayout);
  audioFrame->sample_rate = sampleRate;

  audioFrame->nb_samples = numSamples;
  if(av_frame_get_buffer(audioFrame, 0) < 0)
  {
    av_frame_free(&audioFrame);
    throw runtime_error("Video::genAudioFrame: av_frame_get_buffer failed");
  }
  // if(av_frame_make_writable(audioFrame) < 0) { throw runtime_error("Video::genAudioFrame: av_frame_make_writable failed"); }

  return audioFrame;
}

void Video::addFrame(Image const & img)
{
  if(frameCounter == 0)  /// initialize
  {
    /// video initialization
    if(_fmtCtx->oformat->video_codec == AV_CODEC_ID_NONE) { throw runtime_error("no video codec"); }

    AVCodec const * videoCodec = avcodec_find_encoder(_fmtCtx->oformat->video_codec);
    if(!videoCodec) { throw runtime_error("avcodec_find_encoder failed"); }
    l.r(f("codec: %\n") % videoCodec->name);

    _videoCodecContext = avcodec_alloc_context3(videoCodec);
    if(!_videoCodecContext) { throw runtime_error("avcodec_alloc_context3 failed"); }

    _videoCodecContext->codec_id = _fmtCtx->oformat->video_codec;
    _videoCodecContext->bit_rate = _bitRate;

    _videoCodecContext->time_base.num = 1;
    _videoCodecContext->time_base.den = _fps;
    // _videoCodecContext->framerate.num = fps;
    // _videoCodecContext->framerate.den = 1;

    if(isInterframe)
    {
      _videoCodecContext->gop_size     = _gopSize;
      _videoCodecContext->max_b_frames = _max_b_frames;
      // codeContext->rc_min_rate
      // codeContext->rc_max_rate
    }

    _videoStream = avformat_new_stream(_fmtCtx, NULL);
    if(!_videoStream) { throw runtime_error("avformat_new_stream failed"); }
    _videoStream->id = _fmtCtx->nb_streams - 1;

    // _videoStream->time_base.num = 1;
    // _videoStream->time_base.den = fps;
    _videoStream->time_base = _videoCodecContext->time_base;

    _videoFrame = av_frame_alloc();
    if(!_videoFrame) { throw runtime_error("av_frame_alloc failed"); }
    _videoFrame->pts = 0;
    l.r("\n");

    _videoCodecContext->width  = img.width;
    _videoCodecContext->height = img.height;

    if(_fmtCtx->oformat->flags & AVFMT_GLOBALHEADER) _videoCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if(img.numComps == 4) { _srcPixelFormat = AV_PIX_FMT_RGBA; }
    _videoCodecContext->pix_fmt = _dstPixelFormat;

    if(avcodec_open2(_videoCodecContext, videoCodec, &_opts) < 0) { throw runtime_error("avcodec_open2 failed"); }

    // avFrame->width  = outWidth;
    // avFrame->height = outHeight;
    _videoFrame->width  = _videoCodecContext->width;
    _videoFrame->height = _videoCodecContext->height;

    _videoFrame->format = _videoCodecContext->pix_fmt;
    if(av_frame_get_buffer(_videoFrame, 0) < 0) { throw runtime_error("av_frame_get_buffer failed"); }

    if(avcodec_parameters_from_context(_videoStream->codecpar, _videoCodecContext) < 0)
    {
      throw runtime_error("avcodec_parameters_from_context failed");
    }

    /// TODO: consider SWS_BICUBIC for arg after _videoCodecContext->pix_fmt
    _swsContext = sws_getContext(_videoCodecContext->width, _videoCodecContext->height, _srcPixelFormat, _videoFrame->width, _videoFrame->height,
                                 _videoCodecContext->pix_fmt, 0, 0, 0, 0);
    if(!_swsContext) { throw runtime_error("sws_getContext failed"); }

    /// audio initialization
    i32 samplesPerFrame = 0;
    if(hasAudio)
    {
      if(_fmtCtx->oformat->audio_codec == AV_CODEC_ID_NONE) { throw runtime_error("Video::addFrame: format doesn't support audio"); }

#ifndef SYNTH_AUDIO_TEST
      /// in audio
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
#endif

      /// out audio
      AVCodec const * audioCodec = avcodec_find_encoder(_fmtCtx->oformat->audio_codec);
      if(!audioCodec) { throw runtime_error("Video::addFrame: avcodec_find_encoder failed"); }

#ifndef SYNTH_AUDIO_TEST
      bool supportedSampleRate = false;
      for(i32 i = 0; audioCodec->supported_samplerates[i]; i++)
      {
        if(audioCodec->supported_samplerates[i] == inAudioCodecContext->sample_rate)
        {
          supportedSampleRate = true;
          break;
        }
      }
      if(!supportedSampleRate) { throw runtime_error("Video::addFrame: unsupported sample rate"); }
#endif

      _audioCodecContext = avcodec_alloc_context3(audioCodec);

      _audioCodecContext->sample_fmt = audioCodec->sample_fmts ? audioCodec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;

#ifdef SYNTH_AUDIO_TEST

      _audioCodecContext->sample_rate =  audioSampleRate;

      _audioCodecContext->time_base.num = 1;
      _audioCodecContext->time_base.den = audioSampleRate;

#else
      
      // _audioCodecContext->sample_fmt = inAudioCodecContext->sample_fmt;

      _audioCodecContext->sample_rate = inAudioCodecContext->sample_rate;
      // _audioCodecContext->bit_rate     = inAudioCodecContext->bit_rate;

      _audioCodecContext->time_base.num = 1;
      _audioCodecContext->time_base.den = inAudioCodecContext->sample_rate;  //_fps
#endif

      AVChannelLayout outChannelLayout;
      outChannelLayout.order       = AV_CHANNEL_ORDER_NATIVE;
      outChannelLayout.u.mask      = AV_CH_LAYOUT_STEREO;
      outChannelLayout.nb_channels = 2;
      av_channel_layout_copy(&_audioCodecContext->ch_layout, &outChannelLayout);

      /// NOTE: avcodec_open2 sets the frame_size unless audioCodec->capabilities has AV_CODEC_CAP_VARIABLE_FRAME_SIZE
      if(avcodec_open2(_audioCodecContext, audioCodec, &_opts) < 0) { throw runtime_error("Video::addFrame: avcodec_open2 failed"); }

      _audioStream     = avformat_new_stream(_fmtCtx, nullptr);
      _audioStream->id = _fmtCtx->nb_streams - 1;

      _audioStream->time_base = _audioCodecContext->time_base;

      if(avcodec_parameters_from_context(_audioStream->codecpar, _audioCodecContext) < 0)
      {
        throw runtime_error("Video::addFrame: avcodec_parameters_from_context failed");
      }

      if(audioCodec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE) { samplesPerFrame = 10000; }
      else { samplesPerFrame = _audioCodecContext->frame_size; }

      if(_fmtCtx->oformat->flags & AVFMT_GLOBALHEADER) _audioCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

#ifdef SYNTH_AUDIO_TEST
      if(swr_alloc_set_opts2(&_swrContext, &_audioCodecContext->ch_layout, _audioCodecContext->sample_fmt, _audioCodecContext->sample_rate,
                             &_audioCodecContext->ch_layout, audioSampleFormat, _audioCodecContext->sample_rate, 0, nullptr) < 0)
#else
      if(swr_alloc_set_opts2(&_swrContext, &_audioCodecContext->ch_layout, _audioCodecContext->sample_fmt, _audioCodecContext->sample_rate,
                             &inAudioCodecContext->ch_layout, inAudioCodecContext->sample_fmt, inAudioCodecContext->sample_rate, 0, nullptr) < 0)

      // av_assert0(_audioCodecContext->sample_rate == inAudioCodecContext->sample_rate);
#endif
      {
        throw runtime_error("Video::addFrame: swr_alloc_set_opts2 failed");
      }

      if(swr_init(_swrContext) < 0) { throw runtime_error("Video::addFrame: swr_init failed"); }

      /// transcode audio frames into memory
      i32 sampleCount     = 0;
      AVPacket * inPacket = av_packet_alloc();
      if(!inPacket) { throw runtime_error("av_packet_alloc failed"); }

      vector<AVFrame *> inFrames;

#ifdef SYNTH_AUDIO_TEST
      f32 t       = 0;
      f32 tincr   = 2 * M_PI * 110.0 / _audioCodecContext->sample_rate;
      f32 tincr2  = 2 * M_PI * 110.0 / _audioCodecContext->sample_rate / _audioCodecContext->sample_rate;
      i64 nextPts = 0;

      AVFrame * inFrame = genAudioFrame(audioSampleFormat, outChannelLayout, audioSampleRate, samplesPerFrame);
  
      if(!inFrame) { throw runtime_error("genAudioFrame failed"); }

      // while(true)
      // {
      //   if(av_compare_ts(nextPts, _audioCodecContext->time_base, STREAM_DURATION, {1, 1}) >= 0) 
      //   { 
      //     break; 
      //   }

      for(u32 i_ = 0; i_ < _numAudioFrames; i_++)
      {
        int j, i, v;
        int16_t * q = (int16_t *)inFrame->data[0];

        for(j = 0; j < inFrame->nb_samples; j++)
        {
          v = (int)(sin(t) * 10000);
          for(i = 0; i < inFrame->ch_layout.nb_channels; i++)
            *q++ = v;
          t += tincr;
          tincr += tincr2;
        }

        inFrame->pts = nextPts;
        nextPts += inFrame->nb_samples;

        AVFrame * audioFrame = genAudioFrame(_audioCodecContext->sample_fmt, _audioCodecContext->ch_layout, _audioCodecContext->sample_rate, samplesPerFrame);
    
        if(av_frame_make_writable(audioFrame) < 0) { throw runtime_error("Video::genAudioFrame: av_frame_make_writable failed"); }

        if(swr_convert(_swrContext, audioFrame->data, audioFrame->nb_samples, (uint8_t const **)inFrame->data, inFrame->nb_samples) < 0)
        {
          throw runtime_error("Video::addFrame: swr_convert failed");
        }

        audioFrame->pts = sampleCount;
        sampleCount += audioFrame->nb_samples;

        _audioFrames.emplace_back(audioFrame);

        // inFrames.push_back(inFrame);
        av_frame_unref(inFrame);
      }

#else
      while(true)
      {
        i32 ret = av_read_frame(inAudioFmtCtx, inPacket);
        if(ret < 0)
        {
          if(ret == AVERROR_EOF)  /// end of file reached
          {
            break;
          }
          else { throw runtime_error("Video::addFrame: av_read_frame failed"); }
        }

        if(inPacket->stream_index != audioStreamIdx)
        {
          av_packet_unref(inPacket);
          continue;
        }

        decode(inAudioCodecContext, inFrames, inPacket);

        AVFrame * audioFrame = genAudioFrame(_audioCodecContext->format, outChannelLayout, _audioCodecContext->sample_rate, samplesPerFrame);
        // genAudioFrame(i32 format, AVChannelLayout const & outChannelLayout, i32 sampleRate, i32 numSamples)

        i64 delay = swr_get_delay(_swrContext, inAudioCodecContext->sample_rate);
        i64 dstNumSamples   = av_rescale_rnd(delay + inFrame->nb_samples, _audioCodecContext->sample_rate, inAudioCodecContext->sample_rate, AV_ROUND_UP);
        av_assert0(dstNumSamples == audioFrame->nb_samples); /// ???

        if(swr_convert(_swrContext, audioFrame->data, dstNumSamples, (uint8_t const **)inFrame->data, inFrame->nb_samples) < 0)
        {
          throw runtime_error("Video::addFrame: swr_convert failed");
        }

        audioFrame->pts = av_rescale_q(sampleCount, inAudioCodecContext->time_base, _audioCodecContext->time_base);  /// this is always sampleCount
        sampleCount += audioFrame->nb_samples;

        _audioFrames.emplace_back(audioFrame);

        av_packet_unref(inPacket);
      }

      /// flush the decoder
      decode(inAudioCodecContext, inFrames, nullptr);
#endif

      // /// flush the transcoder
      // AVFrame * audioFrame = genAudioFrame(inAudioCodecContext, samplesPerFrame);
      // swr_convert(_swrContext, audioFrame->data, _audioCodecContext->frame_size, 0, 0);
      // audioFrame->pts = sampleCount;
      // _audioFrames.emplace_back(audioFrame);

      /// clean up in audio
      for(AVFrame * inFrame : inFrames)
      {
        av_frame_free(&inFrame);
      }
      inFrames.clear();

#ifndef SYNTH_AUDIO_TEST
      avcodec_free_context(&inAudioCodecContext);
      avformat_free_context(inAudioFmtCtx);
#endif

      av_packet_free(&inPacket);      

    }  /// end of audio input decoding

    // l.i("__ av dump format __");
    av_dump_format(_fmtCtx, 0, _outPath.c_str(), 1);  // NOTE: just a printout
    l.r("\n");

    /// open the file
    if(avio_open(&_fmtCtx->pb, _outPath.c_str(), AVIO_FLAG_WRITE) < 0) { throw runtime_error("avio_open failed"); }
    if(avformat_write_header(_fmtCtx, &_opts) < 0)  /// &opt
    {
      throw runtime_error("avformat_write_header failed");
    }

    /// done initializing

  }  /// end of 1st frame setup

  /// encode video frame
  if(av_frame_make_writable(_videoFrame) < 0) { throw runtime_error("av_frame_make_writable failed"); }

  u8 const * const * srcSlice = &img.data;
  i32 const * srcStride       = reinterpret_cast<i32 const *>(&img.pitch);
  if(sws_scale(_swsContext, srcSlice, srcStride, 0, _videoCodecContext->height, _videoFrame->data, _videoFrame->linesize) < 0)
  {
    throw runtime_error("Video::addFrame: sws_scale failed");
  }

  _videoFrame->pts = frameCounter++;

  encode(_fmtCtx, _videoFrame, _videoStream, _videoCodecContext);

  if(hasAudio && _audioFrames.size() > 0)
  {
    AVFrame * audioFrame = _audioFrames.front();
    encode(_fmtCtx, audioFrame, _audioStream, _audioCodecContext);
    av_frame_free(&audioFrame);
    _audioFrames.erase(_audioFrames.begin());
  }
}

void Video::write()
{
  encode(_fmtCtx, nullptr, _videoStream, _videoCodecContext);

  if(hasAudio) encode(_fmtCtx, nullptr, _audioStream, _audioCodecContext);

  /// cleanup
  av_dict_free(&_opts);

  av_write_trailer(_fmtCtx);

  avcodec_free_context(&_videoCodecContext);
  av_frame_free(&_videoFrame);
  sws_freeContext(_swsContext);

  if(hasAudio)
  {
    avcodec_free_context(&_audioCodecContext);

    for(auto & frame : _audioFrames)  /// any left over frames
    {
      av_frame_free(&frame);
    }
    _audioFrames.clear();

    if(_swrContext) swr_free(&_swrContext);
  }

  if(!(_fmtCtx->oformat->flags & AVFMT_NOFILE)) avio_closep(&_fmtCtx->pb);

  avformat_free_context(_fmtCtx);

  this->_written = true;
}