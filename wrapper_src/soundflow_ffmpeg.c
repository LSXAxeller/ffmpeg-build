#include "soundflow_ffmpeg.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IO_BUFFER_SIZE 32768

// Internal Structs

struct SF_Decoder {
    AVFormatContext* format_ctx;
    AVCodecContext* codec_ctx;
    AVPacket* packet;
    AVFrame* frame;
    SwrContext* swr_ctx;
    int stream_index;
    AVIOContext* avio_ctx;
    uint8_t* io_buffer;
    sf_read_callback onRead;
    sf_seek_callback onSeek;
    void* pUserData;
    int target_bytes_per_sample;
    int target_channels;
};

struct SF_Encoder {
    AVFormatContext* format_ctx;
    AVCodecContext* codec_ctx;
    AVStream* stream;
    AVPacket* packet;
    AVFrame* frame;
    SwrContext* swr_ctx;
    AVIOContext* avio_ctx;
    uint8_t* io_buffer;
    sf_write_callback onWrite;
    void* pUserData;
    int64_t next_pts;
    SFSampleFormat input_format;
};

// Helper Functions

static enum AVSampleFormat to_ffmpeg_sample_format(SFSampleFormat format) {
    switch (format) {
        case SF_SAMPLE_FORMAT_U8:  return AV_SAMPLE_FMT_U8;
        case SF_SAMPLE_FORMAT_S16: return AV_SAMPLE_FMT_S16;
        // FFmpeg uses 32-bit containers for 24-bit audio, which is fine.
        case SF_SAMPLE_FORMAT_S24: return AV_SAMPLE_FMT_S32;
        case SF_SAMPLE_FORMAT_S32: return AV_SAMPLE_FMT_S32;
        case SF_SAMPLE_FORMAT_F32: return AV_SAMPLE_FMT_FLT;
        default: return AV_SAMPLE_FMT_NONE;
    }
}

static SFSampleFormat from_ffmpeg_sample_format(enum AVSampleFormat format) {
    switch (format) {
        case AV_SAMPLE_FMT_U8:
        case AV_SAMPLE_FMT_U8P:  return SF_SAMPLE_FORMAT_U8;
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S16P: return SF_SAMPLE_FORMAT_S16;
        case AV_SAMPLE_FMT_S32:
        case AV_SAMPLE_FMT_S32P: return SF_SAMPLE_FORMAT_S32;
        case AV_SAMPLE_FMT_FLT:
        case AV_SAMPLE_FMT_FLTP: return SF_SAMPLE_FORMAT_F32;
        // FFmpeg does not support a native packed 24-bit format.
        default: return SF_SAMPLE_FORMAT_UNKNOWN;
    }
}

// I/O Callbacks

static int read_packet_callback(void* opaque, uint8_t* buf, int buf_size) {
    SF_Decoder* decoder = (SF_Decoder*)opaque;
    size_t bytes_read = decoder->onRead(decoder->pUserData, buf, buf_size);
    if (bytes_read == 0) {
        return AVERROR_EOF;
    }
    return (int)bytes_read;
}

static int64_t seek_callback_wrapper(void* opaque, int64_t offset, int whence) {
    SF_Decoder* decoder = (SF_Decoder*)opaque;
    // AVSEEK_SIZE is a special request to get the file size.
    if (whence == AVSEEK_SIZE) {
        return -1;
    }
    return decoder->onSeek(decoder->pUserData, offset, whence);
}

static int write_packet_callback(void* opaque, uint8_t* buf, int buf_size) {
    SF_Encoder* encoder = (SF_Encoder*)opaque;
    return (int)encoder->onWrite(encoder->pUserData, buf, buf_size);
}


//  Decoder Implementation

SF_FFMPEG_API SF_Decoder* sf_decoder_create() {
    return (SF_Decoder*)calloc(1, sizeof(SF_Decoder));
}

SF_FFMPEG_API int sf_decoder_init(SF_Decoder* decoder, sf_read_callback onRead, sf_seek_callback onSeek, void* pUserData,
                                 SFSampleFormat target_format, SFSampleFormat* out_native_format,
                                 uint32_t* out_channels, uint32_t* out_samplerate) {
    if (!decoder) return -1;
    decoder->onRead = onRead;
    decoder->onSeek = onSeek;
    decoder->pUserData = pUserData;

    decoder->format_ctx = avformat_alloc_context();
    if (!decoder->format_ctx) return -1;
    decoder->io_buffer = (uint8_t*)av_malloc(IO_BUFFER_SIZE);
    if (!decoder->io_buffer) { avformat_free_context(decoder->format_ctx); return -1; }

    decoder->avio_ctx = avio_alloc_context(decoder->io_buffer, IO_BUFFER_SIZE, 0, decoder, read_packet_callback, NULL, seek_callback_wrapper);
    if (!decoder->avio_ctx) { av_free(decoder->io_buffer); avformat_free_context(decoder->format_ctx); return -1; }
    decoder->format_ctx->pb = decoder->avio_ctx;

    if (avformat_open_input(&decoder->format_ctx, NULL, NULL, NULL) != 0) return -2;
    if (avformat_find_stream_info(decoder->format_ctx, NULL) < 0) return -3;

    decoder->stream_index = av_find_best_stream(decoder->format_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (decoder->stream_index < 0) return -4;

    AVStream* stream = decoder->format_ctx->streams[decoder->stream_index];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) return -5;

    decoder->codec_ctx = avcodec_alloc_context3(codec);
    if (!decoder->codec_ctx) return -5;

    avcodec_parameters_to_context(decoder->codec_ctx, stream->codecpar);
    if (avcodec_open2(decoder->codec_ctx, codec, NULL) < 0) return -6;

    *out_channels = decoder->codec_ctx->ch_layout.nb_channels;
    *out_samplerate = decoder->codec_ctx->sample_rate;
    *out_native_format = from_ffmpeg_sample_format(decoder->codec_ctx->sample_fmt);

    enum AVSampleFormat target_av_format = to_ffmpeg_sample_format(target_format);
    if (target_av_format == AV_SAMPLE_FMT_NONE) return -7; // Invalid target format

    decoder->target_bytes_per_sample = av_get_bytes_per_sample(target_av_format);
    decoder->target_channels = decoder->codec_ctx->ch_layout.nb_channels;

    // Setup resampler to convert from native format to the requested target format
    swr_alloc_set_opts2(&decoder->swr_ctx,
                        &decoder->codec_ctx->ch_layout, target_av_format, decoder->codec_ctx->sample_rate,
                        &decoder->codec_ctx->ch_layout, decoder->codec_ctx->sample_fmt, decoder->codec_ctx->sample_rate,
                        0, NULL);
    if (!decoder->swr_ctx || swr_init(decoder->swr_ctx) < 0) return -8;

    decoder->packet = av_packet_alloc();
    decoder->frame = av_frame_alloc();
    if (!decoder->packet || !decoder->frame) return -9;

    return 0;
}

SF_FFMPEG_API int64_t sf_decoder_get_length_in_pcm_frames(SF_Decoder* decoder) {
    if (!decoder || !decoder->format_ctx || decoder->stream_index < 0) return 0;
    AVStream* stream = decoder->format_ctx->streams[decoder->stream_index];
    if (stream->duration != AV_NOPTS_VALUE) {
        return av_rescale_q(stream->duration, stream->time_base, (AVRational){1, stream->sample_rate});
    }
    // Fallback for formats without duration info (e.g. WAV)
    if (decoder->format_ctx->duration != AV_NOPTS_VALUE) {
        return av_rescale(decoder->format_ctx->duration, AV_TIME_BASE, stream->sample_rate);
    }
    return 0;
}

SF_FFMPEG_API int64_t sf_decoder_read_pcm_frames(SF_Decoder* decoder, void* pFramesOut, int64_t frameCount) {
    uint8_t* out_ptr[] = { (uint8_t*)pFramesOut };
    int64_t frames_read = 0;

    while (frames_read < frameCount) {
        int ret = avcodec_receive_frame(decoder->codec_ctx, decoder->frame);
        if (ret == 0) { // Success, we have a frame
            int64_t frames_to_convert = frameCount - frames_read;
            int out_samples = swr_convert(decoder->swr_ctx, out_ptr, (int)frames_to_convert, (const uint8_t**)decoder->frame->data, decoder->frame->nb_samples);
            if (out_samples > 0) {
                out_ptr[0] += out_samples * decoder->target_channels * decoder->target_bytes_per_sample;
                frames_read += out_samples;
            }
            av_frame_unref(decoder->frame);
            continue;
        }

        if (ret == AVERROR(EAGAIN)) { // Need more data
            av_packet_unref(decoder->packet);
            if (av_read_frame(decoder->format_ctx, decoder->packet) != 0) {
                avcodec_send_packet(decoder->codec_ctx, NULL); // Flush decoder
                continue;
            }
            if (decoder->packet->stream_index == decoder->stream_index) {
                if (avcodec_send_packet(decoder->codec_ctx, decoder->packet) != 0) break;
            }
            av_packet_unref(decoder->packet);
            continue;
        }
        break; // EOF or an error, break the loop
    }
    return frames_read;
}

SF_FFMPEG_API int sf_decoder_seek_to_pcm_frame(SF_Decoder* decoder, int64_t frameIndex) {
    if (!decoder || !decoder->format_ctx || decoder->stream_index < 0) return -1;
    AVStream* stream = decoder->format_ctx->streams[decoder->stream_index];
    int64_t timestamp = av_rescale_q(frameIndex, (AVRational){1, stream->sample_rate}, stream->time_base);
    // Flush internal buffers before seeking
    avcodec_flush_buffers(decoder->codec_ctx);
    return av_seek_frame(decoder->format_ctx, decoder->stream_index, timestamp, AVSEEK_FLAG_BACKWARD);
}

SF_FFMPEG_API void sf_decoder_free(SF_Decoder* decoder) {
    if (!decoder) return;
    avcodec_free_context(&decoder->codec_ctx);

    // Custom IO context needs special handling for freeing
    if (decoder->format_ctx) {
        if (decoder->format_ctx->pb) {
            av_freep(&decoder->format_ctx->pb->buffer);
            avio_context_free(&decoder->format_ctx->pb);
        }
        avformat_close_input(&decoder->format_ctx);
    }

    av_packet_free(&decoder->packet);
    av_frame_free(&decoder->frame);
    swr_free(&decoder->swr_ctx);
    free(decoder);
}


// Encoder Implementation

SF_FFMPEG_API SF_Encoder* sf_encoder_create() {
    return (SF_Encoder*)calloc(1, sizeof(SF_Encoder));
}

static int encode_and_write(SF_Encoder* encoder, AVFrame* frame) {
    int ret = avcodec_send_frame(encoder->codec_ctx, frame);
    if (ret < 0) return ret;

    while (ret >= 0) {
        ret = avcodec_receive_packet(encoder->codec_ctx, encoder->packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return 0;
        } else if (ret < 0) {
            return ret; // Error
        }
        av_packet_rescale_ts(encoder->packet, encoder->codec_ctx->time_base, encoder->stream->time_base);
        encoder->packet->stream_index = encoder->stream->index;
        av_interleaved_write_frame(encoder->format_ctx, encoder->packet);
        av_packet_unref(encoder->packet);
    }
    return 0;
}

SF_FFMPEG_API int sf_encoder_init(SF_Encoder* encoder, const char* format_name, sf_write_callback onWrite, void* pUserData, SFSampleFormat sampleFormat, uint32_t channels, uint32_t sampleRate) {
    if (!encoder) return -1;
    encoder->onWrite = onWrite;
    encoder->pUserData = pUserData;
    encoder->next_pts = 0;
    encoder->input_format = sampleFormat;

    const AVOutputFormat* out_fmt = av_guess_format(format_name, NULL, NULL);
    if (!out_fmt) return -2; // Format not found

    avformat_alloc_output_context2(&encoder->format_ctx, out_fmt, NULL, NULL);
    if (!encoder->format_ctx) return -2;

    const AVCodec* codec = avcodec_find_encoder(out_fmt->audio_codec);
    if (!codec) return -3; // Codec for this format not found

    encoder->stream = avformat_new_stream(encoder->format_ctx, codec);
    if (!encoder->stream) return -3;
    encoder->codec_ctx = avcodec_alloc_context3(codec);
    if (!encoder->codec_ctx) return -3;

    // Set parameters
    encoder->codec_ctx->channels = channels;
    AVChannelLayout ch_layout = AV_CHANNEL_LAYOUT_STEREO;
    if (channels == 1) ch_layout = AV_CHANNEL_LAYOUT_MONO;
    av_channel_layout_copy(&encoder->codec_ctx->ch_layout, &ch_layout);
    encoder->codec_ctx->sample_rate = sampleRate;
    encoder->codec_ctx->time_base = (AVRational){1, sampleRate};

    // Choose the best sample format the encoder supports
    if (codec->sample_fmts) {
        encoder->codec_ctx->sample_fmt = codec->sample_fmts[0];
    } else {
        return -4; // Encoder supports no sample formats
    }

    if (encoder->format_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        encoder->codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (avcodec_open2(encoder->codec_ctx, codec, NULL) < 0) return -5;
    if (avcodec_parameters_from_context(encoder->stream->codecpar, encoder->codec_ctx) < 0) return -6;

    encoder->io_buffer = (uint8_t*)av_malloc(IO_BUFFER_SIZE);
    encoder->avio_ctx = avio_alloc_context(encoder->io_buffer, IO_BUFFER_SIZE, 1, encoder, NULL, write_packet_callback, NULL);
    encoder->format_ctx->pb = encoder->avio_ctx;

    if (avformat_write_header(encoder->format_ctx, NULL) < 0) return -7;

    // Setup resampler to convert from the provided input format to the encoder's required format
    enum AVSampleFormat input_av_format = to_ffmpeg_sample_format(sampleFormat);
    if(input_av_format == AV_SAMPLE_FMT_NONE) return -8;

    swr_alloc_set_opts2(&encoder->swr_ctx,
                        &encoder->codec_ctx->ch_layout, encoder->codec_ctx->sample_fmt, encoder->codec_ctx->sample_rate,
                        &encoder->codec_ctx->ch_layout, input_av_format, sampleRate, 0, NULL);
    if (!encoder->swr_ctx || swr_init(encoder->swr_ctx) < 0) return -9;

    encoder->packet = av_packet_alloc();
    encoder->frame = av_frame_alloc();
    if (!encoder->packet || !encoder->frame) return -10;

    return 0;
}

SF_FFMPEG_API int64_t sf_encoder_write_pcm_frames(SF_Encoder* encoder, void* pFramesIn, int64_t frameCount) {
    AVFrame* resampled_frame = av_frame_alloc();
    resampled_frame->format = encoder->codec_ctx->sample_fmt;
    resampled_frame->ch_layout = encoder->codec_ctx->ch_layout;
    resampled_frame->sample_rate = encoder->codec_ctx->sample_rate;
    resampled_frame->nb_samples = (int)frameCount;
    if (av_frame_get_buffer(resampled_frame, 0) < 0) {
        av_frame_free(&resampled_frame);
        return -1;
    }

    const uint8_t* pIn[] = { (const uint8_t*)pFramesIn };
    swr_convert(encoder->swr_ctx, resampled_frame->data, resampled_frame->nb_samples, pIn, (int)frameCount);

    resampled_frame->pts = encoder->next_pts;
    encoder->next_pts += resampled_frame->nb_samples;
    encode_and_write(encoder, resampled_frame);
    av_frame_free(&resampled_frame);
    return frameCount;
}

SF_FFMPEG_API void sf_encoder_free(SF_Encoder* encoder) {
    if (!encoder) return;

    // Flush the encoder by sending a NULL frame
    encode_and_write(encoder, NULL);

    av_write_trailer(encoder->format_ctx);

    avcodec_free_context(&encoder->codec_ctx);
    if (encoder->format_ctx) {
        if (encoder->format_ctx->pb) {
            av_freep(&encoder->format_ctx->pb->buffer);
            avio_context_free(&encoder->format_ctx->pb);
        }
        avformat_free_context(encoder->format_ctx);
    }

    av_packet_free(&encoder->packet);
    av_frame_free(&encoder->frame);
    swr_free(&encoder->swr_ctx);

    free(encoder);
}