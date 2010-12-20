#include "image_spliter.h"

void getTextureLayout(FFmpegContext *ctx, int *tw, int *th)
{
    int i = 0;
    int c = 0;
    while(c < ctx->videoCtx->width)
    {
        c += 64;
        i++;
    }
    *tw = i;

    i = 0;
    c = 0;

    while(c < ctx->videoCtx->height)
    {
        c += 64;
        i++;
    }
    *th = i;
}


uint8_t* getTexture(uint8_t* data, int limit, int linesize, int tw, int th)
{
    const int len = 64 * 64 * 4;
    uint8_t *out = malloc(len);
    uint8_t *src = data;
    uint8_t *split = out;

    memset(split, 0, len);

    int base = (th * 64 * linesize) + tw * 64 * 4;
    int total = base;
    src += base;
    int i, j;
    for(i = 0; i < 64; i++)
    {
        if(total + linesize < limit)
        {
            memcpy(split, src, 64 * 4);
        }
       
        src += linesize;
        total += linesize;
        split += (64 * 4);
    }

    return out;
}
