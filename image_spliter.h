#ifndef _IMAGE_SPLITER_H
#define _IMAGE_SPLITER_H

#include "ffmpeg_context.h"
void getTextureLayout(FFmpegContext *ctx, int *tw, int *th);
uint8_t* getTexture(uint8_t* data, int limit, int linesize, int tw, int th);
#endif
