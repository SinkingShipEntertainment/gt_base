

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

// #define SYNTH_AUDIO_TEST
u32 synthBitRate                      = 64000;
u32 synthSampleRate                   = 44100;
i32 synthSamplesPerFrame              = 1152;  /// 1152 samples per frame for mp3
enum AVSampleFormat synthSampleFormat = AV_SAMPLE_FMT_S16;

bool encode(AVFormatContext * fmtCtx, AVFrame const * avFrame, AVStream * avStream, AVCodecContext * codecContext)
{
  if(avcodec_send_frame(codecContext, avFrame) < 0) { throw runtime_error("avcodec_send_frame failed"); }

  while(true)
  {
    AVPacket avPacket = {0};

    /// for video returns AVERROR(EAGAIN) when a frame is sent to avcodec_send_frame, when avFrame is null it returns above 0
    i32 ret = avcodec_receive_packet(codecContext, &avPacket);
    if(ret == AVERROR(EAGAIN))
    {
      av_packet_unref(&avPacket);
      return false;
    }
    if(ret == AVERROR_EOF)
    {
      av_packet_unref(&avPacket);
      return true;
    }

    // avPacket.time_base = codecContext->time_base;

    if(ret < 0) { throw runtime_error("encode: avcodec_receive_packet failed"); }

    /// for videeo only get here on the final flush

    av_packet_rescale_ts(&avPacket, codecContext->time_base, avStream->time_base);
    avPacket.stream_index = avStream->index;

    ret = av_interleaved_write_frame(fmtCtx, &avPacket);
    av_packet_unref(&avPacket);
    if(ret < 0) { throw runtime_error("av_interleaved_write_frame failed"); }
  }

  return false;
}

void decode(AVCodecContext * codecContext, AVPacket const * avPacket, vector<AVFrame *> & frames, i64 & nextPts)
{
  if(avcodec_send_packet(codecContext, avPacket) < 0) { throw runtime_error("Video::decode avcodec_send_packet failed"); }

  while(true)
  {
    AVFrame * inFrame = av_frame_alloc();

    i32 ret = avcodec_receive_frame(codecContext, inFrame);
    if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
    {
      av_frame_free(&inFrame);
      return;
    }
    if(ret < 0) { throw runtime_error("Video::decode avcodec_receive_frame failed"); }

    if(codecContext->frame_size != inFrame->nb_samples)  /// initial nb_samples is sometimes less and this screws the encoding up (will hang or crash)
    {
      av_frame_free(&inFrame);
      continue;
    }

    nextPts += inFrame->nb_samples;
    frames.emplace_back(inFrame);
  }
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
  return audioFrame;
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

Video::Video(string const & outPath,
             string const & avFormatStr,
             i32 const fps,
             u32 crf,
             string const & preset,
             i64 const bitRate,
             i32 const gopSize,
             i32 const max_b_frames)
    : frameCounter(0),
      hasAudio(false),
      isInterframe(true),  /// interframe or intraframe: h264 is but dnxhd for example is not
      _written(false),
      _outPath(outPath),
      _avFormatStr(avFormatStr),
      _fps(fps),
      _crf(crf),
      _preset(preset),
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
      _dstPixelFormat(AV_PIX_FMT_YUV444P),  /// AV_PIX_FMT_YUV420P AV_PIX_FMT_YUV422P AV_PIX_FMT_YUV444P
      _bt709ColorSpace(false),
      _swrContext(nullptr),
      _audioCodecContext(nullptr),
      _audioStream(nullptr),
      _audioDuration(0)
// _audioFrameIndex(0)
{
  // throw runtime_error("omg Video failed");

  av_log_set_callback(&avLogCb);

  string containerStr(avFormatStr);

  enum AVCodecID avCodecId = AV_CODEC_ID_NONE;

  if(containerStr == "h264")
  {
    avCodecId        = AV_CODEC_ID_H264;
    containerStr     = "mov";  /// mov uses h264 via lib264
    _bt709ColorSpace = true;
  }
  else if(containerStr == "h265")
  {
    avCodecId    = AV_CODEC_ID_H265;
    containerStr = "mov";
  }
  else if(containerStr == "dnxhd")  /// dnxhd is intraframe
  {
    isInterframe = false;
  }
  else
  {
    l.e(f("unsupported format: %") % containerStr);
    return;
  }

  if(avformat_alloc_output_context2(&_fmtCtx, NULL, containerStr.c_str(), _outPath.c_str()) < 0)
  {
    throw runtime_error("avformat_alloc_output_context2 failed");
  }

  if(avformat_query_codec(_fmtCtx->oformat, avCodecId, FF_COMPLIANCE_NORMAL) < 0) { throw runtime_error(f("unsupported format: %") % containerStr); }

  if(containerStr == "mov")
  {
    string crfStr(f("%") % _crf);
    av_dict_set(&_opts, "crf", crfStr.c_str(), 0);
    av_dict_set(&_opts, "preset", _preset.c_str(), 0);
  }
}

Video::~Video()
{
  if(!this->_written) { this->write(); }
}

/// @brief add audio to the video, currently can only add one audio once per Video instance
/// @param filePath
void Video::addAudio(string const & filePath, u32 numFrames)
{
  // _audioPath = filePath; /// TODO: validate audio file path
  // _audioDuration = static_cast<f32>(numFrames) / static_cast<f32>(_fps);
  // hasAudio = true;
}

void Video::addFrame(Image const & img)
{
  if(frameCounter == 0)  /// initialize
  {
    /// video initialization
    if(_fmtCtx->oformat->video_codec == AV_CODEC_ID_NONE) { throw runtime_error("no video codec"); }

    AVCodec const * videoCodec = avcodec_find_encoder(_fmtCtx->oformat->video_codec);
    if(!videoCodec) { throw runtime_error("avcodec_find_encoder failed"); }
    l.d(f("videoCodec->name: %\n") % videoCodec->name);

    _videoCodecContext = avcodec_alloc_context3(videoCodec);
    if(!_videoCodecContext) { throw runtime_error("avcodec_alloc_context3 failed"); }

    _videoCodecContext->codec_id = _fmtCtx->oformat->video_codec;
    _videoCodecContext->bit_rate = _bitRate;

    _videoCodecContext->time_base.num = 1;
    _videoCodecContext->time_base.den = _fps;

    if(isInterframe)
    {
      _videoCodecContext->gop_size     = _gopSize;
      _videoCodecContext->max_b_frames = _max_b_frames;
    }

    _videoStream = avformat_new_stream(_fmtCtx, NULL);
    if(!_videoStream) { throw runtime_error("avformat_new_stream failed"); }
    _videoStream->id = _fmtCtx->nb_streams - 1;

    _videoStream->time_base = _videoCodecContext->time_base;

    _videoFrame = av_frame_alloc();
    if(!_videoFrame) { throw runtime_error("av_frame_alloc failed"); }
    _videoFrame->pts = 0;

    if(_bt709ColorSpace)
    {
      _videoFrame->colorspace      = AVCOL_SPC_BT709;
      _videoFrame->color_primaries = AVCOL_PRI_BT709;
      _videoFrame->color_trc       = AVCOL_TRC_BT709;
      _videoFrame->color_range     = AVCOL_RANGE_JPEG;
      _videoFrame->chroma_location = AVCHROMA_LOC_LEFT;
    }

    l.r("\n");

    _videoCodecContext->width  = img.width;
    _videoCodecContext->height = img.height;

    if(_fmtCtx->oformat->flags & AVFMT_GLOBALHEADER) _videoCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if(img.numComps == 4) { _srcPixelFormat = AV_PIX_FMT_RGBA; }
    _videoCodecContext->pix_fmt = _dstPixelFormat;

    _videoCodecContext->colorspace             = _videoFrame->colorspace;
    _videoCodecContext->color_primaries        = _videoFrame->color_primaries;
    _videoCodecContext->color_trc              = _videoFrame->color_trc;
    _videoCodecContext->color_range            = _videoFrame->color_range;
    _videoCodecContext->chroma_sample_location = _videoFrame->chroma_location;

    if(avcodec_open2(_videoCodecContext, videoCodec, &_opts) < 0) { throw runtime_error("avcodec_open2 failed"); }
    // if(avcodec_open2(_videoCodecContext, videoCodec, nullptr) < 0) { throw runtime_error("avcodec_open2 failed"); }

    _videoFrame->width  = _videoCodecContext->width;
    _videoFrame->height = _videoCodecContext->height;

    _videoFrame->format = _videoCodecContext->pix_fmt;
    if(av_frame_get_buffer(_videoFrame, 0) < 0) { throw runtime_error("av_frame_get_buffer failed"); }

    if(avcodec_parameters_from_context(_videoStream->codecpar, _videoCodecContext) < 0)
    {
      throw runtime_error("avcodec_parameters_from_context failed");
    }

    _swsContext = sws_getContext(_videoCodecContext->width, _videoCodecContext->height, _srcPixelFormat, _videoFrame->width, _videoFrame->height,
                                 _videoCodecContext->pix_fmt, 0, 0, 0, 0);
    if(!_swsContext) { throw runtime_error("sws_getContext failed"); }

    if(_bt709ColorSpace)
    {
      i32 in_full, out_full, brightness, contrast, saturation;
      i32 const *inv_table, *table;

      sws_getColorspaceDetails(_swsContext, (i32 **)&inv_table, &in_full, (i32 **)&table, &out_full, &brightness, &contrast, &saturation);

      table = sws_getCoefficients(AVCOL_SPC_BT709);

      out_full = _videoCodecContext->color_range;

      sws_setColorspaceDetails(_swsContext, inv_table, in_full, table, out_full, brightness, contrast, saturation);
    }

    /// audio initialization

    if(hasAudio)
    {
      if(_fmtCtx->oformat->audio_codec == AV_CODEC_ID_NONE) { throw runtime_error("Video::addFrame: format doesn't support audio"); }

      // #ifndef SYNTH_AUDIO_TEST
      /// in audio
      AVFormatContext * inAudioFmtCtx = nullptr;
      if(avformat_open_input(&inAudioFmtCtx, _audioPath.c_str(), NULL, NULL) < 0)
      {
        throw runtime_error("Video::addFrame: avformat_open_input failed");
      }
      if(avformat_find_stream_info(inAudioFmtCtx, NULL) < 0) { throw runtime_error("Video::addFrame: avformat_find_stream_info failed"); }

      AVCodec const * inAudioCodec = nullptr;
      i32 audioStreamId            = av_find_best_stream(inAudioFmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, &inAudioCodec, 0);
      if(audioStreamId < 0) { throw runtime_error("Video::addFrame: av_find_best_stream failed"); }

      AVStream * inAudioAvStream = inAudioFmtCtx->streams[audioStreamId];

      AVCodecContext * inAudioCodecContext = avcodec_alloc_context3(inAudioCodec);
      if(!inAudioCodecContext) { throw runtime_error("Video::addFrame: avcodec_alloc_context3 failed"); }
      avcodec_parameters_to_context(inAudioCodecContext, inAudioAvStream->codecpar);
      if(avcodec_open2(inAudioCodecContext, inAudioCodec, &_opts) < 0) { throw runtime_error("Video::addFrame: avcodec_open2 failed"); }
      /// end of input audio
      // #endif

      /// out audio
      AVCodec const * audioCodec = avcodec_find_encoder(_fmtCtx->oformat->audio_codec);
      if(!audioCodec) { throw runtime_error("Video::addFrame: avcodec_find_encoder failed"); }

      _audioCodecContext             = avcodec_alloc_context3(audioCodec);
      _audioCodecContext->sample_fmt = audioCodec->sample_fmts ? audioCodec->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;

      i32 srcSampleRate = 0;
      i64 srcBitRate    = 0;

#ifdef SYNTH_AUDIO_TEST
      srcSampleRate = synthSampleRate;
      srcBitRate    = synthBitRate;
#else
      srcSampleRate = inAudioCodecContext->sample_rate;
      srcBitRate    = inAudioCodecContext->bit_rate;
#endif

      bool supportedSampleRate = false;
      for(i32 i = 0; audioCodec->supported_samplerates[i]; i++)
      {
        if(audioCodec->supported_samplerates[i] == srcSampleRate)
        {
          supportedSampleRate = true;
          break;
        }
      }
      if(!supportedSampleRate) { throw runtime_error("Video::addFrame: unsupported sample rate"); }

      _audioCodecContext->sample_rate   = srcSampleRate;
      _audioCodecContext->bit_rate      = srcBitRate;
      _audioCodecContext->time_base.num = 1;
      _audioCodecContext->time_base.den = srcSampleRate;

      AVChannelLayout outChannelLayout;
      outChannelLayout.order       = AV_CHANNEL_ORDER_NATIVE;
      outChannelLayout.u.mask      = AV_CH_LAYOUT_STEREO;
      outChannelLayout.nb_channels = 2;
      av_channel_layout_copy(&_audioCodecContext->ch_layout, &outChannelLayout);

      /// NOTE: avcodec_open2 sets the frame_size unless audioCodec->capabilities has AV_CODEC_CAP_VARIABLE_FRAME_SIZE
      if(avcodec_open2(_audioCodecContext, audioCodec, nullptr) < 0) { throw runtime_error("Video::addFrame: avcodec_open2 failed"); }

      _audioStream     = avformat_new_stream(_fmtCtx, nullptr);
      _audioStream->id = _fmtCtx->nb_streams - 1;

      _audioStream->time_base = _audioCodecContext->time_base;

      if(avcodec_parameters_from_context(_audioStream->codecpar, _audioCodecContext) < 0)
      {
        throw runtime_error("Video::addFrame: avcodec_parameters_from_context failed");
      }

      i32 maxSamplesPerFrame = 0;
      if(audioCodec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE) { maxSamplesPerFrame = 10000; }
      else { maxSamplesPerFrame = _audioCodecContext->frame_size; }

      if(_fmtCtx->oformat->flags & AVFMT_GLOBALHEADER) _audioCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

#ifdef SYNTH_AUDIO_TEST
      if(swr_alloc_set_opts2(&_swrContext, &_audioCodecContext->ch_layout, _audioCodecContext->sample_fmt, _audioCodecContext->sample_rate,
                             &_audioCodecContext->ch_layout, synthSampleFormat, _audioCodecContext->sample_rate, 0, nullptr) < 0)
#else
      if(swr_alloc_set_opts2(&_swrContext, &_audioCodecContext->ch_layout, _audioCodecContext->sample_fmt, _audioCodecContext->sample_rate,
                             &inAudioCodecContext->ch_layout, inAudioCodecContext->sample_fmt, inAudioCodecContext->sample_rate, 0, nullptr) < 0)
#endif
      {
        throw runtime_error("Video::addFrame: swr_alloc_set_opts2 failed");
      }

      if(swr_init(_swrContext) < 0) { throw runtime_error("Video::addFrame: swr_init failed"); }

      vector<AVFrame *> inFrames;

#ifdef SYNTH_AUDIO_TEST  /// generate audio frames
      f32 t            = 0;
      f32 tincr        = 2 * M_PI * 110.0 / _audioCodecContext->sample_rate;
      f32 const tincr2 = 2 * M_PI * 110.0 / _audioCodecContext->sample_rate / _audioCodecContext->sample_rate;
      i64 nextPts      = 0;

      while(true)
      {
        if(av_compare_ts(nextPts, _audioCodecContext->time_base, _audioDuration, {1, 1}) > 0) { break; }

        AVFrame * inFrame = genAudioFrame(synthSampleFormat, outChannelLayout, synthSampleRate, synthSamplesPerFrame);
        if(!inFrame) { throw runtime_error("genAudioFrame failed"); }

        i32 j, i, v;
        i16 * q = reinterpret_cast<i16 *>(inFrame->data[0]);
        for(j = 0; j < inFrame->nb_samples; j++)
        {
          v = static_cast<i32>(sin(t) * 10000);
          for(i = 0; i < inFrame->ch_layout.nb_channels; i++)
            *q++ = v;
          t += tincr;
          tincr += tincr2;
        }

        inFrame->pts = nextPts;
        nextPts += inFrame->nb_samples;

        inFrames.emplace_back(inFrame);
      }

#else  /// read audio frames from input file

      AVCodecParserContext * parser = av_parser_init(inAudioCodec->id);
      if(!parser) { throw runtime_error("av_parser_init failed"); }

      AVPacket * inPacket = av_packet_alloc();
      if(!inPacket) { throw runtime_error("av_packet_alloc failed"); }

      i64 nextPts = 0;
      i32 ret     = 0;
      // u8 data[AVCODEC_MAX_AUDIO_FRAME_SIZE];
      while(true)
      {
        if(av_compare_ts(nextPts, _audioCodecContext->time_base, _audioDuration, {1, 1}) > 0) { break; }

        // ret = av_parser_parse2(parser, inAudioContext, &inPacket->data, &inPacket->size,
        //                        data, data_size,
        //                        AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);

        ret = av_read_frame(inAudioFmtCtx, inPacket);
        if(ret < 0)
        {
          if(ret == AVERROR_EOF)  /// end of file reached
          {
            break;
          }
          else { throw runtime_error("Video::addFrame: av_read_frame failed"); }
        }

        if(inPacket->stream_index != audioStreamId)
        {
          av_packet_unref(inPacket);
          continue;
        }

        decode(inAudioCodecContext, inPacket, inFrames, nextPts);
      }

      /// flush the decoder
      decode(inAudioCodecContext, nullptr, inFrames, nextPts);

      av_packet_free(&inPacket);

#endif

      avcodec_free_context(&inAudioCodecContext);
      avformat_free_context(inAudioFmtCtx);

      // i32 maxDestSamples = av_rescale_rnd(src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);

      i32 sampleCount = 0;
      for(auto const & inFrame : inFrames)
      {
        // i64 delay = swr_get_delay(_swrContext, srcSampleRate);
        // i64 dstNumSamples   = av_rescale_rnd(delay + inFrame->nb_samples, _audioCodecContext->sample_rate, srcSampleRate, AV_ROUND_UP);

        // if(inFrame->nb_samples < maxSamplesPerFrame) continue; /// TODO: cull this earlier

        i32 dstNumSamples = maxSamplesPerFrame;
        // i32 dstNumSamples = swr_get_out_samples(_swrContext, inFrame->nb_samples);
        // if(dstNumSamples > maxSamplesPerFrame) dstNumSamples = maxSamplesPerFrame;

        AVFrame * audioFrame =
            genAudioFrame(_audioCodecContext->sample_fmt, _audioCodecContext->ch_layout, _audioCodecContext->sample_rate, dstNumSamples);

        if(av_frame_make_writable(audioFrame) < 0) { throw runtime_error("Video::genAudioFrame: av_frame_make_writable failed"); }

        i32 ret = swr_convert(_swrContext, audioFrame->data, dstNumSamples, (u8 const **)inFrame->data, inFrame->nb_samples);
        if(ret < 0) { throw runtime_error("Video::addFrame: swr_convert failed"); }
        // audioFrame->nb_samples = ret;

        audioFrame->pts = sampleCount;
        sampleCount += audioFrame->nb_samples;

        _audioFrames.emplace_back(audioFrame);
      }

      /// flush the transcoder
      // AVFrame * audioFrame = genAudioFrame(_audioCodecContext->sample_fmt, _audioCodecContext->ch_layout, _audioCodecContext->sample_rate,
      // maxSamplesPerFrame); ret = swr_convert(_swrContext, audioFrame->data, _audioCodecContext->frame_size, 0, 0); if(ret > 0)
      // {
      //   audioFrame->pts = sampleCount;
      //   _audioFrames.emplace_back(audioFrame);
      // }
      // else
      // {
      //   av_frame_free(&audioFrame);
      // }

      /// clean up in audio
      for(AVFrame * inFrame : inFrames)
      {
        av_frame_free(&inFrame);
      }
      inFrames.clear();

    }  /// end of audio input decoding

    if(_avFormatStr == "h264") av_dict_set(&_fmtCtx->metadata, "encoder", "H.264", 0);

    /// open the file
    if(avio_open(&_fmtCtx->pb, _outPath.c_str(), AVIO_FLAG_WRITE) < 0) { throw runtime_error("avio_open failed"); }

    /// enforce framerate
    _videoStream->avg_frame_rate = {static_cast<i32>(_fps), 1};  /// NOTE: RV requires this othwerwise it will use calculate the framerate incorrectly

    /// the following call recomputes_videoStream->time_base
    if(avformat_write_header(_fmtCtx, &_fmtCtx->metadata) < 0)  /// &opt
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

  frameCounter++;

  encode(_fmtCtx, _videoFrame, _videoStream, _videoCodecContext);
  _videoFrame->pts = frameCounter;

  if(hasAudio && _audioFrames.size() > 0)
  {
    while(true)  /// add some audio frames
    {
      if(_audioFrames.size() == 0) { break; }

      AVFrame * audioFrame = _audioFrames.front();
      // AVFrame * audioFrame = _audioFrames[_audioFrameIndex++];
      if(av_compare_ts(audioFrame->pts, _audioCodecContext->time_base, _videoFrame->pts, _videoCodecContext->time_base) > 0) { break; }

      encode(_fmtCtx, audioFrame, _audioStream, _audioCodecContext);
      av_frame_free(&audioFrame);
      _audioFrames.erase(_audioFrames.begin());
    }
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

    for(auto & frame : _audioFrames)
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
