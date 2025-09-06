#ifndef SOUNDFLOW_FFMPEG_H
#define SOUNDFLOW_FFMPEG_H

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#define SF_FFMPEG_API __declspec(dllexport)
#else
#define SF_FFMPEG_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handles
typedef struct SF_Decoder SF_Decoder;
typedef struct SF_Encoder SF_Encoder;

typedef enum {
  SF_SAMPLE_FORMAT_UNKNOWN = 0,
  SF_SAMPLE_FORMAT_U8 = 1,
  SF_SAMPLE_FORMAT_S16 = 2,
  SF_SAMPLE_FORMAT_S24 = 3,
  SF_SAMPLE_FORMAT_S32 = 4,
  SF_SAMPLE_FORMAT_F32 = 5,
} SFSampleFormat;

// Callbacks for custom I/O
typedef size_t (*sf_read_callback)(void* pUserData, void* pBuffer,
                                   size_t bytesToRead);
typedef int64_t (*sf_seek_callback)(void* pUserData, int64_t offset,
                                    int whence);
typedef size_t (*sf_write_callback)(void* pUserData, void* pBuffer,
                                    size_t bytesToWrite);

// Decoder Functions
SF_FFMPEG_API SF_Decoder* sf_decoder_create();
SF_FFMPEG_API int sf_decoder_init(
    SF_Decoder* decoder, sf_read_callback onRead, sf_seek_callback onSeek,
    void* pUserData,
    SFSampleFormat target_format,       // The target output format
    SFSampleFormat* out_native_format,  // The original format of the file
    uint32_t* out_channels, uint32_t* out_samplerate);
SF_FFMPEG_API int64_t sf_decoder_get_length_in_pcm_frames(SF_Decoder* decoder);
SF_FFMPEG_API int64_t sf_decoder_read_pcm_frames(SF_Decoder* decoder,
                                                 void* pFramesOut,
                                                 int64_t frameCount);
SF_FFMPEG_API int sf_decoder_seek_to_pcm_frame(SF_Decoder* decoder,
                                               int64_t frameIndex);
SF_FFMPEG_API void sf_decoder_free(SF_Decoder* decoder);

// Encoder Functions
SF_FFMPEG_API SF_Encoder* sf_encoder_create();
// format_name is e.g. "mp3", "flac", "wav", "opus"
SF_FFMPEG_API int sf_encoder_init(SF_Encoder* encoder, const char* format_name,
                                  sf_write_callback onWrite, void* pUserData,
                                  SFSampleFormat sampleFormat,
                                  uint32_t channels, uint32_t sampleRate);
SF_FFMPEG_API int64_t sf_encoder_write_pcm_frames(SF_Encoder* encoder,
                                                  void* pFramesIn,
                                                  int64_t frameCount);
SF_FFMPEG_API void sf_encoder_free(SF_Encoder* encoder);

#ifdef __cplusplus
}
#endif

#endif  // SOUNDFLOW_FFMPEG_H