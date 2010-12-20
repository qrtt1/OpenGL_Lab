/*
 *  ffmpeg_context.h
 *  
 *
 *  Created by Apple on 2010/9/14.
 *  Copyright 2010 Muzee. All rights reserved.
 *
 */

#ifndef __FFMPEG_CONTEXT_H
#define __FFMPEG_CONTEXT_H

#include <pthread.h>
#include "libavformat/avformat.h"
//#include "libavcore/audioconvert.h"
#include "libswscale/swscale.h"
#include "image_converter.h"

#define msleep(t) do{int n=t;usleep(1000 * n);}while(0);
#define IS_READY_TO_USE(ctx) (ctx->status==FC_OK)
#define RESET_CONTEXT_FOR_OPEN(ctx) do{ctx->status=FC_OK;ctx->isOpenStarted=0;}while(0); 


struct AVAudioConvert;
typedef struct AVAudioConvert AVAudioConvert;

enum FFmpegContextStatus{
    FC_OK,
    FC_FILE_NOT_OPEN,
    FC_CANNOT_OPEN_MEDIA_SOURCE,
    FC_NO_ANY_STREAM_EITERH_VIDEO_OR_AUDIO,
    FC_READ_FRAME_ERR,
    FC_ALREADLY_USED
};

//#define DEFAULT_AUDIO_BUFFER_SIZE ((AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2)
#define DEFAULT_AUDIO_BUFFER_SIZE (AVCODEC_MAX_AUDIO_FRAME_SIZE)

////////////////////////////////////////////////////

extern const int RESULT_TYPE_AUDIO;
extern const int RESULT_TYPE_VIDEO;
extern const int RESULT_TYPE_SKIP;

struct VideoInfo
{
    AVPacket *pkt;
    uint8_t* data;
    int size;
    int used;
    int noframe;
    int decoded;
    int key;
    uint64_t pts;
    struct VideoInfo* next;
};

typedef struct
{
    // video
    int width;
    int height;
    float frameRate;

    // audio info
    int sampleFormat;
    int sampleRate;
    int channels;

    int bufferSize;
    char videoCodecName[12];
    char audioCodecName[12];
    uint64_t firstPacketTime;

    float averageBitrate;
    char url[512];

    int isSet;
} MediaInfo;

typedef struct 
{
    /* decoded data */
    void*   data;
    
    /* bytes in data */
    int     bytes;

    /* audio or video */
    int     type;

    /* persentation timestamp */
    uint64_t pts;

    int key;
} AVResult;

typedef struct 
{
//    uint8_t   data[DEFAULT_AUDIO_BUFFER_SIZE];
    DECLARE_ALIGNED(16,uint8_t, data)[DEFAULT_AUDIO_BUFFER_SIZE];
    DECLARE_ALIGNED(16,uint8_t, converted_data)[DEFAULT_AUDIO_BUFFER_SIZE];
    int     converted;
    int     dataSize;
} __AudioBuf;

typedef struct 
{
    AVFormatContext *formatCtx;

    int videoIndex;
    int audioIndex;
    
    AVCodecContext* videoCtx;
    AVCodec* videoCodec;
    AVCodecContext* audioCtx;
    AVCodec* audioCodec;

    //AVPacket packet;
    AVResult result;

    // [VIDEO] decoded frame
    //AVFrame* videoFrame;

    // [AUDIO] fields
    int audioBytesPerSec;
    int16_t* audioDataBuffer;
    int audioDataBufferSize;

    double audioPtsSec;

    __AudioBuf audioBuf;
    MediaInfo info;

    int isOpenStarted;
    int isOpenCompleted;
    int isStreamInfoStarted;
    int isStreamInfoCompleted;
    int isCloseInvoked;
    int isDisableNextFrame;
    int requiredBufferSize;
    int status;

    int txPacket;

    uint64_t filterPts;
    uint64_t cutPts;

    Yuv2RgbContext imageConv;
    AVPacket packet;
    struct VideoInfo* head;

    AVAudioConvert *reformat_ctx;

    long TOTAL_VIDEO_PACKET;
    long TOTAL_FREED_VIDEO_PACKET;
    long TOTAL_VIDEO_DATA;
    long TOTAL_FREED_VIDEO_DATA;
    long outVideoCnt;

    int bVideoThreading;
    pthread_t tVideoThread;
    pthread_mutex_t mGLock;

    uint64_t hitAudio;
    uint64_t hitAudioDiff;
    uint64_t carriedKeyPts;
    uint64_t lastNextFrame;

    void (*logger)(const char* tag, const char* message);
} FFmpegContext;


void init_ffmpeg_context();

void getMediaInfo(FFmpegContext* ctx);

void openMediaSource(
        FFmpegContext *ctx, 
        const char* source,
        void* logger);

void closeMediaSource(
        FFmpegContext *ctx);

void nextFrame(FFmpegContext *ctx);

int doReadPacket(FFmpegContext *ctx, AVPacket *pkt);

#endif
