#ifndef _IMAGE_CONVERTER_H
#define _IMAGE_CONVERTER_H

#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

typedef struct
{
    struct SwsContext *converter;
    AVFrame *rgbFrame;
    uint8_t* rgbBuffer;
    int rgbBufferSize;
    int hasOpen;
} Yuv2RgbContext;

Yuv2RgbContext* openYuv2RgbContext(
        Yuv2RgbContext* converter,
        int width, 
        int height);

void closeYuv2RgbContext(
        Yuv2RgbContext* converter);


void yuv2rgb(Yuv2RgbContext context, AVFrame *src, int srcHeight);
/*
void yuv2rgb(const uint8_t* const* yuvData, int yuvLineSize, int height);
            sws_scale(imageConv.converter, 
                    (const uint8_t* const*) 
                    &ctx->videoFrame->data, 
                    ctx->videoFrame->linesize, 
                    0, ctx->videoCtx->height,
                    imageConv.rgbFrame->data, 
                    imageConv.rgbFrame->linesize);
*/

#endif
