/*ckwg +29
 * Copyright 2018 by Kitware, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of Kitware, Inc. nor the names of any contributors may be used
 *    to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 * \brief Implementation file for video input using FFMPEG.
 */

#include "ffmpeg_init.h"
#include "ffmpeg_video_input.h"

#include <vital/types/timestamp.h>
#include <vital/exceptions/io.h>
#include <vital/exceptions/video.h>
#include <vital/util/tokenize.h>
#include <vital/types/image_container.h>

#include <kwiversys/SystemTools.hxx>

#include <mutex>
#include <memory>
#include <vector>
#include <sstream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

namespace kwiver {
namespace arrows {
namespace ffmpeg {

// ------------------------------------------------------------------
// Private implementation class
class ffmpeg_video_input::priv
{
public:
  /// Constructor
  priv() :
    f_format_context(nullptr),
    f_video_index(-1),
    f_data_index(-1),
    f_video_encoding(nullptr),
    f_video_stream(nullptr),
    f_frame(nullptr),
    f_software_context(nullptr),
    f_start_time(-1),
    f_frame_number_offset(0),
    video_path(""),
    frame_advanced(0)
  {
    f_packet.data = nullptr;
  }

  // f_* variables are FFMPEG specific

  AVFormatContext* f_format_context;
  int f_video_index;
  int f_data_index;
  AVCodecContext* f_video_encoding;
  AVStream* f_video_stream;
  AVFrame* f_frame;
  AVPacket f_packet;
  SwsContext* f_software_context;


  // Start time of the stream, to offset the pts when computing the frame number.
  // (in stream time base)
  int64_t f_start_time;

  // Presentation timestamp (in stream time base)
  int64_t f_pts;

  // Some codec/file format combinations need a frame number offset.
  // These codecs have a delay between reading packets and generating frames.
  unsigned f_frame_number_offset;

  // Name of video we opened
  std::string video_path;

  static std::mutex open_mutex;

  // For logging in priv methods
  vital::logger_handle_t logger;

  // Current image frame.
  vital::image_memory_sptr current_image_memory;
  kwiver::vital::image_container_sptr current_image;

  // local state
  int frame_advanced; // This is a boolean check value really

  // ==================================================================
  /*
  * @brief Convert a ffmpeg pixel type to kwiver::vital::image_pixel_traits
  *
  * @return \b A kwiver::vital::image_pixel_traits
  */

  // ==================================================================
  /*
  * @brief Whether the video was opened.
  *
  * @return \b true if video was opened.
  */
  bool is_opened()
  {
    return this->f_start_time != -1;
  }

  // ==================================================================
  /*
  * @brief Open the given video.
  *
  * @return \b true if video was opened.
  */
  bool open(std::string video_name)
  {
    // Open the file
    int err = avformat_open_input(&this->f_format_context, this->video_path.c_str(), NULL, NULL);
    if (err != 0)
    {
      LOG_ERROR(this->logger, "Error " << err << " trying to open " << video_name);
      return false;
    }


    // Get the stream information by reading a bit of the file
    if (avformat_find_stream_info(this->f_format_context, NULL) < 0)
    {
      return false;
    }

    // Find a video stream, and optionally a data stream.
    // Use the first ones we find.
    this->f_video_index = -1;
    this->f_data_index = -1;
    AVCodecContext* codec_context_origin = NULL;
    for (unsigned i = 0; i < this->f_format_context->nb_streams; ++i)
    {
      AVCodecContext *const enc = this->f_format_context->streams[i]->codec;
      if (enc->codec_type == AVMEDIA_TYPE_VIDEO && this->f_video_index < 0)
      {
        this->f_video_index = i;
        codec_context_origin = enc;
      }
      else if (enc->codec_type == AVMEDIA_TYPE_DATA && this->f_data_index < 0)
      {
        this->f_data_index = i;
      }
    }

    if (this->f_video_index < 0)
    {
      LOG_ERROR(this->logger, "Error: could not find a video stream in " << this->video_path);
      return false;
    }

    if (this->f_data_index < 0)
    {
      LOG_INFO(this->logger, "No data stream available, using AVMEDIA_TYPE_UNKNOWN stream instead");
      // Fallback for the DATA stream if incorrectly coded as UNKNOWN.
      for (unsigned i = 0; i < this->f_format_context->nb_streams; ++i)
      {
        AVCodecContext *enc = this->f_format_context->streams[i]->codec;
        if (enc->codec_type == AVMEDIA_TYPE_UNKNOWN)
        {
          this->f_data_index = i;
        }
      }
    }

    av_dump_format(this->f_format_context, 0, this->video_path.c_str(), 0);

    // Open the stream
    AVCodec* codec = avcodec_find_decoder(codec_context_origin->codec_id);
    if (!codec)
    {
      LOG_ERROR(this->logger,
        "Error: Codec " << codec_context_origin->codec_descriptor
        << " (" << codec_context_origin->codec_id << ") not found");
      return false;
    }

    // Copy context
    this->f_video_encoding = avcodec_alloc_context3(codec);
    if (avcodec_copy_context(this->f_video_encoding, codec_context_origin) != 0)
    {
      LOG_ERROR(this->logger, "Error: Could not copy codec " << this->f_video_encoding->codec_id);
      return false;
    }

    // Open codec
    if (avcodec_open2(this->f_video_encoding, codec, NULL) < 0)
    {
      LOG_ERROR(this->logger, "Error: Could not open codec " << this->f_video_encoding->codec_id);
      return false;
    }

    this->f_video_stream = this->f_format_context->streams[this->f_video_index];
    this->f_frame = av_frame_alloc();

    if (this->f_video_stream->start_time == int64_t(1) << 63)
    {
      this->f_start_time = 0;
    }
    else
    {
      this->f_start_time = this->f_video_stream->start_time;
    }

    // The MPEG 2 codec has a latency of 1 frame when encoded in an AVI
    // stream, so the pts of the last packet (stored in pts) is
    // actually the next frame's pts.
    if (this->f_video_stream->codec->codec_id == AV_CODEC_ID_MPEG2VIDEO &&
      std::string("avi") == this->f_format_context->iformat->name)
    {
      this->f_frame_number_offset = 1;
    }

    // Not sure if this does anything, but no harm either
    av_init_packet(&this->f_packet);
    this->f_packet.data = nullptr;
    this->f_packet.size = 0;

    return true;
  }

  // ==================================================================
  /*
  * @brief Advance to the next frame (but don't acquire an image).
  *
  * @return \b true if video was valid and we found a frame.
  */
  virtual bool advance()
  {
    // Quick return if the file isn't open.
    if (!this->is_opened())
    {
      this->frame_advanced = 0.;
      return false;
    }

    // \todo - num_frames not implemented
    //// See the comment in num_frames().  This is to make sure that once
    //// we start reading frames, we'll never try to march to the end to
    //// figure out how many frames there are.
    //if (is_->num_frames_ == -2) {
    //  is_->num_frames_ = -1;
    //}

    if (this->f_packet.data)
    {
      av_free_packet(&this->f_packet);  // free previous packet
    }
    this->frame_advanced = 0;

    // \todo - metada not implemented yet
    // clear the metadata from the previous frame
    //this->metadata.clear();

    while (this->frame_advanced == 0 && av_read_frame(this->f_format_context, &this->f_packet) >= 0)
    {
      // Make sure that the packet is from the actual video stream.
      if (this->f_packet.stream_index == this->f_video_index)
      {
        int err = avcodec_decode_video2(this->f_video_encoding,
          this->f_frame, &this->frame_advanced,
          &this->f_packet);
        if (err == AVERROR_INVALIDDATA)
        {// Ignore the frame and move to the next
          av_free_packet(&this->f_packet);
          continue;
        }
        if (err < 0)
        {
          LOG_ERROR(this->logger, "vidl_ffmpeg_istream: Error decoding packet");
          av_free_packet(&this->f_packet);
          return false;
        }

        this->f_pts = av_frame_get_best_effort_timestamp(this->f_frame);
        if (this->f_pts == AV_NOPTS_VALUE)
        {
          this->f_pts = 0;
        }
      }
      // \todo - No metadata support yet
      //// grab the metadata from this packet if from the metadata stream
      //else if (this->packet.stream_index == this->data_index)
      //{
      //  is_->metadata_.insert(is_->metadata_.end(), is_->packet_.data,
      //    is_->packet_.data + is_->packet_.size);
      //}

      if (!this->frame_advanced)
      {
        av_free_packet(&this->f_packet);
      }
    }

    // From ffmpeg apiexample.c: some codecs, such as MPEG, transmit the
    // I and P frame with a latency of one frame. You must do the
    // following to have a chance to get the last frame of the video.
    if (!this->frame_advanced)
    {
      av_init_packet(&this->f_packet);
      this->f_packet.data = nullptr;
      this->f_packet.size = 0;

      int err = avcodec_decode_video2(this->f_video_encoding,
        this->f_frame, &this->frame_advanced,
        &this->f_packet);
      if (err >= 0)
      {
        this->f_pts += static_cast<int64_t>(this->stream_time_base_to_frame());
      }
    }

    // The cached frame is out of date, whether we managed to get a new
    // frame or not.
    this->current_image_memory = nullptr;

    if (!this->frame_advanced)
    {
      this->f_frame->data[0] = NULL;
    }

    return static_cast<bool>(this->frame_advanced);
  }

  // ==================================================================
  /*
  * @brief Get the current timestamp
  *
  * @return \b Current timestamp.
  */
  double current_pts() const
  {
    return this->f_pts * av_q2d(this->f_video_stream->time_base);
  }

  // ==================================================================
  /*
  * @brief Returns the double value to convert from a stream time base to
  *  a frame number
  */
  double stream_time_base_to_frame() const
  {
    if (this->f_video_stream->avg_frame_rate.num == 0.0)
    {
      return av_q2d(av_inv_q(av_mul_q(this->f_video_stream->time_base,
        this->f_video_stream->r_frame_rate)));
    }
    return av_q2d(
      av_inv_q(
        av_mul_q(this->f_video_stream->time_base, this->f_video_stream->avg_frame_rate)));
  }

  bool is_valid() const
  {
    return this->f_frame && this->f_frame->data[0];
  }

  // ==================================================================
  /*
  * @brief Return the current frame number
  *
  * @return \b Current frame number.
  */
  unsigned int frame_number() const
  {
    // Quick return if the stream isn't open.
    if (!this->is_valid())
    {
      return static_cast<unsigned int>(-1);
    }

    return static_cast<unsigned int>(
      (this->f_pts - this->f_start_time) / this->stream_time_base_to_frame()
      - static_cast<int>(this->f_frame_number_offset));
  }

}; // end of internal class.

// static open interlocking mutex
std::mutex ffmpeg_video_input::priv::open_mutex;


// ==================================================================
ffmpeg_video_input
::ffmpeg_video_input()
  : d( new priv() )
{
  attach_logger( "ffmpeg_video_input" ); // get appropriate logger
  d->logger = this->logger();
  ffmpeg_init();
}


ffmpeg_video_input
::~ffmpeg_video_input()
{
  this->close();
}


// ------------------------------------------------------------------
// Get this algorithm's \link vital::config_block configuration block \endlink
vital::config_block_sptr
ffmpeg_video_input
::get_configuration() const
{
  // get base config from base class
  vital::config_block_sptr config = vital::algo::video_input::get_configuration();

  return config;
}


// ------------------------------------------------------------------
// Set this algorithm's properties via a config block
void
ffmpeg_video_input
::set_configuration(vital::config_block_sptr in_config)
{
  // Starting with our generated vital::config_block to ensure that assumed values are present
  // An alternative is to check for key presence before performing a get_value() call.

  vital::config_block_sptr config = this->get_configuration();
  config->merge_config(in_config);
}


// ------------------------------------------------------------------
bool
ffmpeg_video_input
::check_configuration(vital::config_block_sptr config) const
{
  bool retcode(true); // assume success

  return retcode;
}


// ------------------------------------------------------------------
void
ffmpeg_video_input
::open( std::string video_name )
{
  // Close any currently opened file
  this->close();

  d->video_path = video_name;

  {
    std::lock_guard< std::mutex > lock(d->open_mutex);

    if (!kwiversys::SystemTools::FileExists(d->video_path))
    {
      // Throw exception
      throw kwiver::vital::file_not_found_exception(video_name, "File not found");
    }

    if (!d->open(video_name))
    {
      throw kwiver::vital::video_runtime_exception("Video stream open failed for unknown reasons");
    }
  }
}


// ------------------------------------------------------------------
void
ffmpeg_video_input
::close()
{
  if (d->f_packet.data) {
    av_free_packet(&d->f_packet);  // free last packet
  }

  if (d->f_frame)
  {
    av_freep(&d->f_frame);
  }
  d->f_frame = nullptr;

  if (d->f_video_encoding && d->f_video_encoding->opaque)
  {
    av_freep(&d->f_video_encoding->opaque);
  }

  //d->num_frames_ = -2;
  //is_->contig_memory_ = 0;
  d->f_video_index = -1;
  d->f_data_index = -1;
  d->f_start_time = -1;
  d->video_path = "";
  d->frame_advanced = 0;
  //is_->metadata_.clear();
  if (d->f_video_stream)
  {
    avcodec_close(d->f_video_stream ->codec);
    d->f_video_stream = nullptr;
  }
  if (d->f_format_context)
  {
    avformat_close_input(&d->f_format_context);
    d->f_format_context = nullptr;
  }

  d->f_video_encoding = nullptr;
}

// ------------------------------------------------------------------
bool
ffmpeg_video_input
::next_frame( kwiver::vital::timestamp& ts,
              uint32_t timeout )
{
  if (!d->is_opened())
  {
    throw vital::file_not_read_exception(d->video_path, "Video not open");
  }

  bool ret = d->advance();
  if (ret)
  {
    ts = this->frame_timestamp();
  }
  return ret;
}

// ------------------------------------------------------------------
bool ffmpeg_video_input::seek_frame(kwiver::vital::timestamp& ts,
  kwiver::vital::timestamp::frame_t frame_number,
  uint32_t timeout)
{
  // Quick return if the stream isn't open.
  if (!d->is_opened())
  {
    throw vital::file_not_read_exception(d->video_path, "Video not open");
    return false;
  }
  if (frame_number <= 0)
  {
    return false;
  }

  // Quick return if the stream isn't open.
  if (timeout != 0)
  {
    LOG_WARN(this->logger(), "Timeout argument is not supported.");
  }

  int current_frame_number = d->frame_number() + d->f_frame_number_offset + 1;
  // If current frame number is greater than requested frame reopen
  // file to reset to start
  if (current_frame_number > frame_number)
  {
    this->open(d->video_path); // Calls close on current video
    current_frame_number = d->frame_number() + d->f_frame_number_offset + 1;
  }

  // Just advance video until the requested frame is reached
  for (int i = current_frame_number; i < frame_number; ++i)
  {
    if (!d->advance())
    {
      return false;
    }
  }

  ts = this->frame_timestamp();
  return true;
}


// ------------------------------------------------------------------
kwiver::vital::image_container_sptr
ffmpeg_video_input
::frame_image( )
{
  // Quick return if the stream isn't valid
  if (!d->is_valid())
  {
    return nullptr;
  }

  AVCodecContext* enc = d->f_format_context->streams[d->f_video_index]->codec;

  // If we have not already converted this frame, try to convert it
  if (!d->current_image_memory && d->f_frame->data[0] != 0)
  {
    int width = enc->width;
    int height = enc->height;
    int num_pixels = 3;
    vital::image_pixel_traits pixel_trait = vital::image_pixel_traits_of<unsigned char>();
    bool direct_copy = false;

    // If the pixel format is not recognized by then convert the data into RGB_24
    switch (enc->pix_fmt)
    {
      case AV_PIX_FMT_GRAY8:
      {
        num_pixels = 1;
        direct_copy = true;
        break;
      }
      case AV_PIX_FMT_RGBA:
      {
        num_pixels = 4;
        direct_copy = true;
        break;
      }
      case AV_PIX_FMT_MONOWHITE:
      case AV_PIX_FMT_MONOBLACK:
      {
        num_pixels = 1;
        pixel_trait = vital::image_pixel_traits_of<bool>();
        direct_copy = true;
        break;
      }
    }
    if (direct_copy)
    {
      int size = avpicture_get_size(enc->pix_fmt, width, height);
      d->current_image_memory = vital::image_memory_sptr(new vital::image_memory(size));

      AVPicture frame;
      avpicture_fill(&frame, (uint8_t*)d->current_image_memory->data(), enc->pix_fmt, width, height);
      av_picture_copy(&frame, (AVPicture*)d->f_frame, enc->pix_fmt, width, height);
    }
    else
    {
      int size = width * height * num_pixels;
      d->current_image_memory = vital::image_memory_sptr(new vital::image_memory(size));

      d->f_software_context = sws_getCachedContext(
        d->f_software_context,
        width, height, enc->pix_fmt,
        width, height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        NULL, NULL, NULL);

      if (!d->f_software_context)
      {
        LOG_ERROR(this->logger(), "Couldn't create conversion context");
        return nullptr;
      }

      AVPicture rgb_frame;
      avpicture_fill(&rgb_frame, (uint8_t*)d->current_image_memory->data(), AV_PIX_FMT_RGB24, width, height);

      sws_scale(d->f_software_context,
        d->f_frame->data, d->f_frame->linesize,
        0, height,
        rgb_frame.data, rgb_frame.linesize);
    }

    vital::image image(
      d->current_image_memory,
      d->current_image_memory->data(),
      width, height, num_pixels,
      num_pixels, num_pixels * width, num_pixels * width*height
    );
    d->current_image = std::make_shared<vital::simple_image_container>(vital::simple_image_container(image));
  }

  return d->current_image;
}


// ------------------------------------------------------------------
kwiver::vital::timestamp
ffmpeg_video_input
::frame_timestamp() const
{
  if (!this->good())
  {
    return {};
  }

  // We don't always have all components of a timestamp, so start with
  // an invalid TS and add the data we have.
  kwiver::vital::timestamp ts;
  ts.set_frame(d->frame_number() + d->f_frame_number_offset + 1);

  return ts;
}


// ------------------------------------------------------------------
kwiver::vital::metadata_vector
ffmpeg_video_input
::frame_metadata()
{
  LOG_INFO(this->logger(), "Metadata access isn't supported yet");
  return kwiver::vital::metadata_vector();
}


// ------------------------------------------------------------------
kwiver::vital::metadata_map_sptr
ffmpeg_video_input
::metadata_map()
{
  LOG_INFO(this->logger(), "Metadata access isn't supported yet");
  return std::make_shared<kwiver::vital::simple_metadata_map>(
    kwiver::vital::simple_metadata_map());
}


// ------------------------------------------------------------------
bool
ffmpeg_video_input
::end_of_video() const
{
  return false;
}


// ------------------------------------------------------------------
bool
ffmpeg_video_input
::good() const
{
  return d->is_valid() && d->frame_advanced;
}


// ------------------------------------------------------------------
bool
ffmpeg_video_input
::seekable() const
{
  return true;
}

// ------------------------------------------------------------------
size_t
ffmpeg_video_input
::num_frames() const
{
  if (!this->good())
  {
    return -1;
  }
  else if (!this->seekable())
  {
    return 0;
  }

  return -1;
  // \todo: To implement to return the number of frames once the video
  // is seekable.
}

} } } // end namespaces
