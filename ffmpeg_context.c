/*
 *  ffmpeg_context.c
 *  
 *
 *  Created by Apple on 2010/9/14.
 *  Copyright 2010 Muzee. All rights reserved.
 *
 */

#include "logger.h"
#include "ffmpeg_context.h"
#include <pthread.h>

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
static const char* TAG = "FFmpegContext";

const int RESULT_TYPE_AUDIO = 1;
const int RESULT_TYPE_VIDEO = 2;
const int RESULT_TYPE_SKIP = 3;

float __my_fixed_frame_rate = -1;
// private function [START]

void __freePacketIfNeeds(AVPacket packet);
AVFrame* __decodeVideo(FFmpegContext* ctx, AVPacket *pkt);
__AudioBuf* __decodeAudio(FFmpegContext* ctx, AVPacket *pkt);
int seekToFrame(FFmpegContext* ctx);
int estimateBufferSize(FFmpegContext* ctx);

void __default_logger__(const char* tag, const char* message);
// private function [END]

void init_ffmpeg_context() 
{ 
    av_register_all(); 
}

int av_open_delegation(FFmpegContext *ctx, const char* source)
{
    logD(TAG, "av_open: %s", source);
    int ret = av_open_input_file(&ctx->formatCtx, source, NULL, 0, NULL);

    if(ret == 0)
    {
        strncpy(ctx->info.url, source, 512);
        return ret;
    }

    if(strstr(source, "mms") == NULL && strstr(source, "http") == NULL)
        return ret; /* not a mms or http url*/

    char *ptr;
    if((ptr = strstr(source, "://")) == NULL)
        return ret; /* not a url */

    char url[1024];
    char *protos[2] = {"mmsh", "mmst"};

    /* open as mmsh */
    snprintf(url, 1024, "%s%s", protos[0], ptr);
    logI(TAG, "re-open as mmsh: %s", url);
    ret = av_open_input_file(&ctx->formatCtx, url, NULL, 0, NULL);
    if(ret == 0)
    {
        strncpy(ctx->info.url, url, 512);
        return ret;
    }

    /* open as mmst */
    snprintf(url, 1024, "%s%s", protos[1], ptr);
    logI(TAG, "re-open as mmst: %s", url);
    ret = av_open_input_file(&ctx->formatCtx, url, NULL, 0, NULL);
    if(ret == 0)
    {
        strncpy(ctx->info.url, url, 512);
    }

    return ret;
}

void openMediaSource(FFmpegContext *ctx, const char* source, void* logger)
{
    uint64_t firstPacketMeet = av_gettime() / 1000;
    if(ctx->isOpenStarted)
    {
        logD(TAG, "FFMpegContext is already opened. (note: you can not reuse it, even it has been closed.)");
        ctx->status = FC_ALREADLY_USED;
        return ;
    }

    ctx->isOpenStarted = 1;
    ctx->isCloseInvoked = 0;

    /* initialization */
    ctx->cutPts = 0;
    ctx->videoIndex = -1;
    ctx->audioIndex = -1;
    ctx->status = FC_OK;

    ctx->audioBuf.converted = 0;
    ctx->reformat_ctx = NULL;

    ctx->requiredBufferSize = -1;
    ctx->info.isSet = 0;
    ctx->info.firstPacketTime = 0;
    ctx->info.averageBitrate = 0;
    ctx->info.bufferSize = 0;
    ctx->info.channels = 0;
    ctx->info.sampleRate = 0;
    ctx->info.sampleFormat = 0;
    ctx->info.frameRate = 0;
    ctx->info.height = 0;
    ctx->info.width = 0;
    memset(ctx->info.url, 0, 512);

    memset(ctx->info.videoCodecName, 0, 12);
    memset(ctx->info.audioCodecName, 0, 12);
    ctx->txPacket = 0;
    av_init_packet(&ctx->packet);

    ctx->head = NULL;
    ctx->TOTAL_VIDEO_PACKET = 0;
    ctx->TOTAL_FREED_VIDEO_PACKET = 0;
    ctx->TOTAL_VIDEO_DATA = 0;
    ctx->TOTAL_FREED_VIDEO_DATA = 0;
    ctx->outVideoCnt = 0;

    ctx->isStreamInfoStarted = 0;
    ctx->isStreamInfoCompleted = 0;

    ctx->bVideoThreading = 0;
    pthread_mutex_init(&ctx->mGLock, NULL);

    ctx->hitAudio = 0;
    ctx->hitAudioDiff = 0;
    ctx->carriedKeyPts = 0;
    ctx->lastNextFrame = 0;
    ctx->isDisableNextFrame = 0;

    /* fetch stream information **IMPORTANT for fps** */
    ctx->lastNextFrame=(av_gettime()/1000) + 3000; /* mark the close delay timestamp */
    ctx->formatCtx = avformat_alloc_context();
    int ret = av_open_delegation(ctx, source);

    logD(TAG, "open media file, ret: %d", ret);
    if (ret !=0) 
    {
        ctx->status = FC_CANNOT_OPEN_MEDIA_SOURCE;
        return ;
    }

    ctx->lastNextFrame=(av_gettime()/1000) + 3000;/* mark the close delay timestamp */
    ctx->isStreamInfoStarted = 1;
    av_find_stream_info(ctx->formatCtx);
    ctx->isStreamInfoCompleted = 1;
    ctx->info.firstPacketTime = (av_gettime() / 1000 - firstPacketMeet);

    unsigned int i;
    for(i=0; i<ctx->formatCtx->nb_streams; i++)
    {
        if( ctx->formatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO){
            ctx->videoIndex = i;
            logI(TAG, "found video stream");
        }
        if( ctx->formatCtx->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO){
            ctx->audioIndex = i;
            logI(TAG, "found audio stream");
        }
    }

    /**
     * If no stream found, make MtechFFmpegContext NO_ANY_STREAM_EITERH_VIDEO_OR_AUDIO status
     * */
    if (ctx->videoIndex == -1 && ctx->audioIndex == -1) {
        ctx->status = FC_NO_ANY_STREAM_EITERH_VIDEO_OR_AUDIO;
        logI(TAG, "video/audio stream not found");
        return ;
    }

    if (ctx->videoIndex >= 0 /* has video */) {
        logI(TAG, "preparing video context");
        ctx->videoCtx = ctx->formatCtx->streams[ ctx->videoIndex ]->codec;

        logI(TAG, "preparing video codec");
        ctx->videoCodec = avcodec_find_decoder(ctx->videoCtx->codec_id);
        if(ctx->videoCodec == NULL)
        {
            // no codec supported the video stream
            ctx->videoIndex = -1;
            logI(TAG, "unsupported video codec");
        }
        else
        {
            strncpy(ctx->info.videoCodecName, ctx->videoCodec->name, 12);
            ctx->info.averageBitrate = ctx->videoCtx->bit_rate / 1000.0 ;
        }

        if(avcodec_open(ctx->videoCtx, ctx->videoCodec) < 0)
        {
            // get into trouble when opening codec
            ctx->videoIndex = -1;
            logI(TAG, "cannot open video codec");
        }

        /**
         * If we have video stream, to create frame and attach callback functions.
         * */
        if(ctx->videoIndex != -1 /* has video stream*/ )
        {
            logI(TAG, "video resource is set");
        }
    }

    if (ctx->audioIndex >=0 /* has audio */) {
        logI(TAG, "preparing audio context");
        ctx->audioCtx = ctx->formatCtx->streams[ ctx->audioIndex ]->codec;

        logI(TAG, "preparing audio codec");
        ctx->audioCodec = avcodec_find_decoder(ctx->audioCtx->codec_id);
        if(ctx->audioCodec == NULL)
        {
            // no codec supported the audio stream
            ctx->audioIndex = -1;
            logI(TAG, "unsupported audio codec");
        }

        if(avcodec_open(ctx->audioCtx, ctx->audioCodec) < 0)
        {
            // get into trouble when opening codec
            ctx->audioIndex = -1;
            logI(TAG, "cannot open audio codec");
        }
        else
        {
            strncpy(ctx->info.audioCodecName, ctx->audioCodec->name, 12);
            /* set average bit rate when no video */
            if(ctx->videoIndex < 0)
                ctx->info.averageBitrate = ctx->audioCtx->bit_rate / 1000.0 ;
        }

        if(ctx->audioIndex != -1)
        {
            ctx->audioBytesPerSec = ctx->audioCtx->sample_rate * ctx->audioCtx->channels * 2;
            // allocate data buffer
            ctx->audioDataBuffer = av_malloc(sizeof(int16_t) * DEFAULT_AUDIO_BUFFER_SIZE);
            ctx->audioDataBufferSize = DEFAULT_AUDIO_BUFFER_SIZE;
            logI(TAG, "audio codec id: >>%d<<", ctx->audioCodec->id);
            logI(TAG, "audio resource is set");
        }
    }

    logI(TAG, "openMediaSource::end");
    ctx->isOpenCompleted = 1;
}

void closeMediaSource(FFmpegContext *ctx)
{
    logD(TAG, "closeMediaSource invoking.");

    if(!ctx->isOpenCompleted || ctx->isCloseInvoked)
    {
        logD(TAG, "has closed or not open completed.");
        return ;
    }

    ctx->isCloseInvoked = 1;
    ctx->isDisableNextFrame = 1;
    ctx->status = FC_FILE_NOT_OPEN;

    int countDown = 30;
    if(ctx->txPacket)
    {
        countDown *= 2;
        logD(TAG, "warning! read packet does finish.");
    }

    uint64_t wait_time = 0;
    uint64_t tNextFrame = 0;
    int samePts = 0;
    while(countDown > 0)
    {
        wait_time = av_gettime() / 1000 - ctx->lastNextFrame;
        if(ctx->lastNextFrame == tNextFrame)
        {
            samePts ++;
        }
        else
        {
            tNextFrame = ctx->lastNextFrame;
            samePts = 0;
        }

        if(wait_time < 10000 && samePts < 10)
        {
            usleep(1000 * 50);
            countDown -- ;
            logD(TAG, "waiting for a nice time to close media(%lld, %lld, %d)", wait_time, ctx->lastNextFrame, countDown);
            continue ;
        }
        break ;
    }

    if(ctx->videoIndex >= 0 /*has video*/)
    {
        if(ctx->bVideoThreading)
        {
            void * exit_status;
            pthread_mutex_lock(&ctx->mGLock);
            ctx->bVideoThreading = 0;
            pthread_mutex_unlock(&ctx->mGLock);
            logD(TAG, "waiting for thread join");
            pthread_join(ctx->tVideoThread, &exit_status);
            logD(TAG, "thread finished.");
        }
        __asm__("nop;nop;");
        av_free(ctx->videoCtx);
        ctx->videoIndex = -1;
    }

    if(ctx->audioIndex >=0 /*has audio*/)
    {
        __asm__("nop;");
        av_free(ctx->audioDataBuffer);
        ctx->audioDataBuffer = 0;
        av_free(ctx->audioCtx);
        ctx->audioIndex = -1;

        if(ctx->reformat_ctx)
        {
            logD(TAG, "free audio converter.");
            av_audio_convert_free(ctx->reformat_ctx);
        }
    }

    if(ctx->formatCtx)
    {
        __asm__("nop;nop;");
        av_free(ctx->formatCtx);
        ctx->formatCtx = NULL;
    }

    logD(TAG, "closeMediaSource invoked.");
}

void cleanVideoDecodingResources(FFmpegContext* ctx)
{
    /* FREE video informations */
    struct VideoInfo* ptr = ctx->head;
    int fCnt = 0;
    while(ptr != NULL)
    {
        fCnt++;
        av_free_packet(ptr->pkt);
        free(ptr->pkt);
        ctx->TOTAL_FREED_VIDEO_PACKET ++;

        if(ptr->decoded)
        {
            free(ptr->data);
            ctx->TOTAL_FREED_VIDEO_DATA ++;
        }

        struct VideoInfo* self = ptr;
        ptr = ptr->next;
        free(self);
    }
    ctx->head = NULL;

    logD(TAG, "vp: %ld, fvp: %ld", ctx->TOTAL_VIDEO_PACKET, ctx->TOTAL_FREED_VIDEO_PACKET);
    logD(TAG, "vd: %ld, fvd: %ld", ctx->TOTAL_VIDEO_DATA, ctx->TOTAL_FREED_VIDEO_DATA);
    logD(TAG, "used video: %ld", ctx->outVideoCnt);
 
    logD(TAG, "FINISH JOB :: CLEAN RESOURCE (%d)", fCnt);
}

void *videoJob(void * fCtx)
{
    FFmpegContext* myCtx = fCtx;
    openYuv2RgbContext(&myCtx->imageConv, myCtx->videoCtx->width, myCtx->videoCtx->height);
    myCtx->imageConv.hasOpen = 1;
    uint64_t lastFramePts = 0;
    long allDecoding = 1;
    long allLongDecoding = 1;

    while(myCtx->bVideoThreading)
    {
        if(myCtx->head == NULL)
        {
            usleep(1000);
            continue ;
        }

        pthread_mutex_lock(&myCtx->mGLock);
        __asm__("nop;");
        struct VideoInfo* vInfo = myCtx->head;
        uint64_t fPts = myCtx->cutPts;
        while(vInfo != NULL)
        {
            if(!vInfo->decoded && !vInfo->used && !vInfo->noframe)
            {
                if(!vInfo->key && vInfo->pts < fPts)
                {
                    vInfo = vInfo->next;
                    continue ;
                }
                break;
            }
            vInfo = vInfo->next;
        }

        if(vInfo)
        {
            uint64_t dt1 = av_gettime() / 1000;
            AVFrame* frame = __decodeVideo(myCtx, vInfo->pkt);
            uint64_t dt2 = av_gettime() / 1000;

            allDecoding ++;
            if(myCtx->info.frameRate < (dt2 - dt1) * 1.2)
                allLongDecoding ++;

            int drop = 0;
            uint64_t ptsDistance = vInfo->pts - lastFramePts;
            if((double)allLongDecoding / allDecoding < 0.05)
            {
                drop = 0;
            }
            else if((double)allLongDecoding / allDecoding < 0.2)
            {
                drop = ptsDistance < 80;
            }
            else if((double)allLongDecoding / allDecoding < 0.25)
            {
                drop = ptsDistance < 80 || allDecoding % 5 == 0;
            } 
            else if((double)allLongDecoding / allDecoding < 0.3)
            {
                drop = allDecoding  % 5 == 0;
            }
            else
            {
                drop = allDecoding  % 2 || ptsDistance < 60;
            }

            if(drop && !vInfo->key)
            {
                vInfo->noframe = 1;
            }
            else if(frame == NULL)
            {
                vInfo->noframe = 1;    
            }
            else
            {
                lastFramePts = vInfo->pts;
                int size = myCtx->imageConv.rgbFrame->linesize[0] * myCtx->videoCtx->height;

                vInfo->data = malloc(sizeof(uint8_t) * size);
                __asm__("nop;nop;");
                yuv2rgb(myCtx->imageConv, frame, myCtx->videoCtx->height);
                vInfo->size = size;
                __asm__("nop;nop;nop;nop;");
                memcpy(vInfo->data, myCtx->imageConv.rgbFrame->data[0], sizeof(uint8_t) * size);
                vInfo->decoded = 1;

                myCtx->TOTAL_VIDEO_DATA ++;
            }
        }
        pthread_mutex_unlock(&myCtx->mGLock);  

        pthread_mutex_lock(&myCtx->mGLock);  
        if(!myCtx->bVideoThreading) 
        {
            pthread_mutex_unlock(&myCtx->mGLock);  
            break;
        }
        pthread_mutex_unlock(&myCtx->mGLock);  
    }
    closeYuv2RgbContext(&myCtx->imageConv);
    myCtx->imageConv.hasOpen = 0;

    cleanVideoDecodingResources(myCtx);
    pthread_exit(NULL);
}


struct VideoInfo* newPacket(FFmpegContext *ctx, struct VideoInfo* parent, AVPacket avpkt)
{

        ctx->TOTAL_VIDEO_PACKET++;

        AVPacket *pkt = malloc(sizeof(AVPacket));
        if(pkt == NULL)
        {
            logD(TAG, "no memory to malloc AVPacket");
            return NULL;
        }

        int ret = av_new_packet(pkt, avpkt.size);
        if(ret == 0)
        {
            memcpy(pkt->data, avpkt.data, sizeof(uint8_t) * avpkt.size);
            pkt->pts = avpkt.pts;
            pkt->dts = avpkt.dts;
            pkt->flags = avpkt.flags;
        }
        else
        {
            free(pkt);
            logD(TAG, "new AVPacket failure");
            return NULL;
        }

        struct VideoInfo *node = malloc(sizeof(struct VideoInfo));
        if(node == NULL)
        {
            av_free_packet(pkt);
            free(pkt);
            return NULL;
        }

        node->pkt = pkt;
        node->pts = avpkt.pts;
        if(avpkt.pts == AV_NOPTS_VALUE)
            node->pts = avpkt.dts;
        else if(node->pts == AV_NOPTS_VALUE)
            node->pts = 0;
        //
        node->data = NULL;
        node->next = NULL;
        //
        node->used = 0;
        node->decoded = 0;
        node->noframe = 0;
        node->size = 0;
        node->key = ((avpkt.flags & AV_PKT_FLAG_KEY) != 0) ? 1 : 0;
        //
        if(parent)
        {
            parent->next = node;
            logI(TAG, "append %p to %p ", node, parent);
        }

        return node;
}

struct VideoInfo* tail(struct VideoInfo* started)
{
    struct VideoInfo* current = started;
    while(current)
    {
        if(current->next == NULL)
            break ;
        current = current->next;
    }
    return current;
}

void nextFrame(FFmpegContext *ctx)
{
    ctx->lastNextFrame=(av_gettime()/1000);

    if(! ctx->isOpenCompleted )
    {
        ctx->status = FC_FILE_NOT_OPEN;
        return ;
    }

    if(ctx->isDisableNextFrame)
    {
        ctx->status = FC_FILE_NOT_OPEN;
        ctx->result.type = RESULT_TYPE_SKIP;
        return ;
    }

    ctx->status = FC_OK;
    ctx->lastNextFrame=(av_gettime()/1000);

    ctx->txPacket = 1;
    int getPacket = doReadPacket(ctx, &ctx->packet);
    ctx->txPacket = 0;

    if(!getPacket)
    {
        ctx->result.type = RESULT_TYPE_SKIP;
        return ;
    }

    if (ctx->packet.stream_index == ctx->audioIndex)
    {
        logI(TAG, "AUDIO");
        logI(TAG, "decoding audio ...");
        uint64_t dt1 = av_gettime();
        __AudioBuf* audioBuf = __decodeAudio(ctx, &ctx->packet);
        if(audioBuf == NULL)
        {
            logD(TAG, "(audio null)");
            ctx->result.type = RESULT_TYPE_SKIP;
            return ;
        }

        ctx->hitAudio = av_gettime() / 1000;
        logI(TAG, "setting result for audio");

        if(audioBuf->converted)
            ctx->result.data = &audioBuf->converted_data;
        else
            ctx->result.data = &audioBuf->data;

        ctx->result.bytes = audioBuf->dataSize;
        ctx->result.pts = ctx->packet.pts;
        ctx->result.type = RESULT_TYPE_AUDIO; 
        logI(TAG, "audio is decoded.");
        uint64_t dt2 = av_gettime();
        logD(TAG, "audio decoding time: %lld", (dt2 - dt1) / 1000);
        return ;
    }

    if (ctx->packet.stream_index == ctx->videoIndex)
    {
        logI(TAG, "VIDEO");
        if(!ctx->bVideoThreading) 
        {
            ctx->bVideoThreading = 1;
            pthread_create(&ctx->tVideoThread, NULL, videoJob, (void *) ctx);
        }

        /* add video packet to buffer */
        struct VideoInfo* vInfo = NULL;
        if(ctx->head == NULL)
        {
            vInfo = (struct VideoInfo*)newPacket(ctx, NULL, ctx->packet);
            pthread_mutex_lock(&ctx->mGLock);
            ctx->head = vInfo;
            pthread_mutex_unlock(&ctx->mGLock);  
        }
        else
        {
            struct VideoInfo* last = tail(ctx->head);
            vInfo = (struct VideoInfo*)newPacket(ctx, last, ctx->packet);
        }

        /* 清除 pts 過期的部分。找出在過期點之前的最後一個 key frame packet，並以它的前一個為清除點 */
        pthread_mutex_lock(&ctx->mGLock);
        struct VideoInfo* headholder = ctx->head;
        struct VideoInfo* afterSplit = NULL;
        struct VideoInfo* beforeSplit = NULL;
        uint64_t beforeKeyPts = 0;
        vInfo = ctx->head; ctx->head = NULL;

        while(vInfo)
        {
            if(vInfo->pts < ctx->filterPts)
            {
                if(vInfo->key)
                    beforeKeyPts = vInfo->pts;
                beforeSplit = vInfo;
                vInfo = vInfo->next;
                continue ;
            }

            if(vInfo->next && vInfo->next->key)
            {
                afterSplit = vInfo;
                break ;
            }
            vInfo = vInfo->next;
        }
        
        struct VideoInfo* split = NULL;
        if(ctx->carriedKeyPts == beforeKeyPts)
            split = beforeSplit;
        else
            split = afterSplit;


        if(split)
        {
            ctx->head = split->next;
            ctx->cutPts = split->pts;
            split->next = NULL;
            /* free head holder*/
            vInfo = headholder;
            while(vInfo)
            {
                av_free_packet(vInfo->pkt);
                free(vInfo->pkt);
                ctx->TOTAL_FREED_VIDEO_PACKET++;

                if(vInfo->decoded)
                {
                    ctx->TOTAL_FREED_VIDEO_DATA++;
                    free(vInfo->data);
                }
                struct VideoInfo* self = vInfo;
                vInfo = vInfo->next;
                free(self);
            }
        }
        else
        {
            ctx->head = headholder;
        }

        pthread_mutex_unlock(&ctx->mGLock);  

        /* if audio packet is hit long time ago, we find it as sonn as possible */
        ctx->hitAudioDiff = av_gettime() / 1000 - ctx->hitAudio;
        if(ctx->hitAudioDiff > 800)
        {
            ctx->result.type = RESULT_TYPE_SKIP;
            logI(TAG, "find audio (%lld)", ctx->hitAudioDiff);
            return ;
        }

        /* check the video decoded completely */
        if(ctx->head != NULL)
        {
            struct VideoInfo* ptr = ctx->head;
            while(ptr != NULL)
            {
                /* 前面的 filterPts 已經過濾過了，這裡不需要再判斷 */
                if(/*ptr->pkt->pts > ctx->filterPts &&*/ ptr->decoded && !ptr->used)
                {
                    ctx->result.data = ptr->data;
                    ctx->result.bytes = ptr->size;
                    ctx->result.pts = ptr->pts;
                    ctx->result.key = ptr->key;
                    ctx->result.type = RESULT_TYPE_VIDEO;
                    ctx->carriedKeyPts = ptr->pts;
                    ctx->outVideoCnt ++;
                    ptr->used = 1;
                    return ;
                }
                ptr = ptr->next;
            }
        }

        ctx->result.type = RESULT_TYPE_SKIP;
        return ;
    }

}

void getMediaInfo(FFmpegContext* ctx) {
    int bufSize = estimateBufferSize(ctx);
    /*
    int bufSize = seekToFrame(ctx);
    */

    if(!ctx->info.isSet && ctx->isOpenCompleted){
        logD(TAG, "meet first packet in %lld ms", ctx->info.firstPacketTime);
        ctx->info.bufferSize = bufSize;
        if(ctx->videoIndex >= 0 /*set video info*/ )
        {
            logD(TAG, "Video Codec %s", ctx->info.videoCodecName);
            ctx->info.width = ctx->videoCtx->width;
            ctx->info.height = ctx->videoCtx->height;
            logD(TAG, "video size: %dx%d", ctx->info.width, ctx->info.height);

            AVStream *stream = ctx->formatCtx->streams[ ctx->videoIndex ];
            AVRational avFrac = stream->r_frame_rate;
            logD(TAG, "r_frame_rate: %d %d", avFrac.num, avFrac.den);
            if(avFrac.den == 0)
            {
                avFrac = ctx->formatCtx->streams[ ctx->videoIndex ]->avg_frame_rate;
                logD(TAG, "avg_frame_rate: %d %d", avFrac.num, avFrac.den);
            }
            if(avFrac.den != 0)
            {
                ctx->info.frameRate = (avFrac.num/((float)avFrac.den));
                __my_fixed_frame_rate = ctx->info.frameRate;
                logD(TAG, "set fixed frame rate: %f", __my_fixed_frame_rate);
            }

            if(__my_fixed_frame_rate < 0)
            {
                ctx->info.frameRate = 29.7;
                logD(TAG, "set initial frame rate: %f", ctx->info.frameRate);
            }
        }

        if(ctx->audioIndex >= 0 /*set audio info*/ )
        {
            logD(TAG, "Audio Codec %s", ctx->info.audioCodecName);
            
            ctx->info.sampleFormat = ctx->audioCtx->sample_fmt;
            logD(TAG, "format bits per sample: %d", av_get_bits_per_sample_format(ctx->audioCtx->sample_fmt));
            if(ctx->audioCtx->sample_fmt != AV_SAMPLE_FMT_S16 
                    && ctx->audioCtx->sample_fmt != AV_SAMPLE_FMT_U8 )
            {
                ctx->info.sampleFormat = AV_SAMPLE_FMT_S16;
                logD(TAG, "overwrite the audio sample format to PCM 16 bits");
                logD(TAG, "new format bits per sample: %d", av_get_bits_per_sample_format(AV_SAMPLE_FMT_S16));
            }

            ctx->info.sampleRate = ctx->audioCtx->sample_rate;
            logD(TAG, "%d", ctx->info.sampleRate );
            ctx->info.channels = ctx->audioCtx->channels;
        }

        ctx->info.isSet = 1;
        logD(TAG, "media info is set.");
    }
}

void __freePacketIfNeeds(AVPacket packet)
{
    if(packet.data != NULL)
        av_free_packet(&packet);
    packet.data = NULL;
}

AVFrame* __decodeVideo(FFmpegContext* ctx, AVPacket *pkt)
{
    int isCompleted = 0;
    int bytesDecoded = -1;

    AVFrame* vFrame = avcodec_alloc_frame();
    bytesDecoded = avcodec_decode_video2(
            ctx->videoCtx, vFrame, &isCompleted /*true when frame is completely*/, 
            pkt);

    if (bytesDecoded < 0)
    {
        logI(TAG, "video decode error");
        if(vFrame)
            av_free(vFrame);
        return NULL;
    } 

    if (!isCompleted)
    {
        av_free(vFrame);
        return NULL;
    }

    return vFrame;
}

__AudioBuf* __decodeAudio(FFmpegContext* ctx, AVPacket *pkt)
{   

    /* initial bufSize tell audio decoder how many space can use, */
    /* after decoding the bufSize will be the size of audio raw data  */
    int bufSize = 0;
    int bytesDecoded = 0;
    AVPacket _tpkt;
    _tpkt.data = pkt->data;
    _tpkt.size = pkt->size;
    uint8_t* audioData = ctx->audioBuf.data;
    
    int totalBufferSize = 0;
    while(_tpkt.size > 0)
    {
        bufSize = DEFAULT_AUDIO_BUFFER_SIZE;
        bytesDecoded = avcodec_decode_audio3(ctx->audioCtx, (int16_t*)audioData, &bufSize, &_tpkt);
        if(bytesDecoded < 0)
            break ;
        _tpkt.size -= bytesDecoded;
        _tpkt.data += bytesDecoded;
        totalBufferSize += bufSize;
        audioData += (bufSize);

        /*
        logD(TAG, "datasize: %d", bufSize);
        logD(TAG, "len: %d", bytesDecoded);
        logD(TAG, "remaining: %d (%d)", _tpkt.size - bytesDecoded, _tpkt.size);
        */
    }

    //logD(TAG, "audio buf size: %d", totalBufferSize);
    if(bytesDecoded < 0 || totalBufferSize <=0 
        || bufSize > DEFAULT_AUDIO_BUFFER_SIZE)
    {
        ctx->audioBuf.dataSize = 0;
        return NULL;
    }

    if(ctx->audioCtx->sample_fmt != AV_SAMPLE_FMT_S16 
        && ctx->audioCtx->sample_fmt != AV_SAMPLE_FMT_U8 )
    {
        if(!ctx->reformat_ctx)
        {
            ctx->reformat_ctx = av_audio_convert_alloc(
                    AV_SAMPLE_FMT_S16, ctx->info.channels,
                    ctx->audioCtx->sample_fmt, ctx->info.channels,
                    NULL, 0);
            logD(TAG, "create audio converter %p", ctx->reformat_ctx);
        }


        if(ctx->reformat_ctx)
        {
            const void *ibuf[6] = {ctx->audioBuf.data};
            const void *obuf[6] = {ctx->audioBuf.converted_data};
            int istride[6]= {av_get_bits_per_sample_fmt(ctx->audioCtx->sample_fmt)/8};
            int ostride[6]= {2};
            int len = totalBufferSize / istride[0];

            if (av_audio_convert(ctx->reformat_ctx, obuf, ostride, ibuf, istride, len) < 0) {
                /* convert failed. */
                ctx->audioBuf.dataSize = 0;
                return NULL;
            }

            totalBufferSize = len * 2;
            /* mark the audio converted */
            ctx->audioBuf.converted = 1;
        }
    }

    ctx->audioBuf.dataSize = totalBufferSize;
    return &ctx->audioBuf;
}

int doReadPacket(FFmpegContext *ctx, AVPacket* pkt)
{
    if(ctx->status != FC_OK)
        return 0;

    if(ctx->isDisableNextFrame)
        return 0;

    if(ctx->isCloseInvoked || ctx->isDisableNextFrame)
        return 0;

    __freePacketIfNeeds(*pkt);
    if(ctx->status != FC_OK || ctx->formatCtx == NULL)
        return 0;

    int ret = av_read_frame(ctx->formatCtx, pkt);
    if (ret < 0)
    {
        __freePacketIfNeeds(*pkt);
        ctx->status = FC_READ_FRAME_ERR;
        logI(TAG, "cannot read frame");
        return 0;
    }
    return 1;
}



int seekToFrame(FFmpegContext* ctx) {
    if(ctx->status != FC_OK || !ctx->isOpenCompleted)
    {
        logD(TAG, "no media to seek. you sholud open media first");
        return -1;
    }

    if(ctx->requiredBufferSize > 0)
        return ctx->requiredBufferSize;

    if(ctx->videoIndex < 0 /*no video*/)
    {
        /* nextFrame(ctx); */
        ctx->requiredBufferSize = DEFAULT_AUDIO_BUFFER_SIZE;
        return ctx->requiredBufferSize;
    }

    logI(TAG, "seeking to video frame ...");
    /**
     * when we have video stream, we can get video width and height after the frame read.
     */ 
    while(1)
    {

        int bufferSize = avpicture_get_size(PIX_FMT_RGB565, ctx->videoCtx->height, ctx->videoCtx->height);
        if(bufferSize == 0)
        {
            nextFrame(ctx);
            logI(TAG, "SEEK VIDEO FRAME !!!");
            continue ;
        }
        ctx->requiredBufferSize = bufferSize;
        break;
    }

    return MAX(ctx->requiredBufferSize, DEFAULT_AUDIO_BUFFER_SIZE);
}

int estimateBufferSize(FFmpegContext* ctx)
{
    if(ctx->status != FC_OK || !ctx->isOpenCompleted)
    {
        logD(TAG, "no media to seek. you sholud open media first");
        return -1;
    }

    if(ctx->requiredBufferSize > 0)
        return ctx->requiredBufferSize;

    if(ctx->videoIndex > 0)
    {
        int bufferSize = avpicture_get_size(PIX_FMT_RGB565, ctx->videoCtx->height, ctx->videoCtx->height);
        int max = MAX(bufferSize, DEFAULT_AUDIO_BUFFER_SIZE);
        ctx->requiredBufferSize = max;
        return max;
    }

    ctx->requiredBufferSize = DEFAULT_AUDIO_BUFFER_SIZE;
    return ctx->requiredBufferSize;
}
