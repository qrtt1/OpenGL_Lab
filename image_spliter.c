#include "image_spliter.h"

const int TEXTURE_BLOCK_SIZE = 128;

void getTextureLayout(FFmpegContext *ctx, int *tw, int *th)
{
    int i = 0;
    int c = 0;
    while(c < ctx->videoCtx->width)
    {
        c += TEXTURE_BLOCK_SIZE;
        i++;
    }
    *tw = i;

    i = 0;
    c = 0;

    while(c < ctx->videoCtx->height)
    {
        c += TEXTURE_BLOCK_SIZE;
        i++;
    }
    *th = i;
}


uint8_t* getTexture(uint8_t* data, int limit, int linesize, int tw, int th)
{
    const int len = TEXTURE_BLOCK_SIZE * TEXTURE_BLOCK_SIZE * 4;
    uint8_t *out = malloc(len);
    uint8_t *src = data;
    uint8_t *split = out;

    memset(split, 0, len);

    int base = (th * TEXTURE_BLOCK_SIZE * linesize) + tw * TEXTURE_BLOCK_SIZE * 4;
    int total = base;
    src += base;
    int i;
    for(i = 0; i < TEXTURE_BLOCK_SIZE; i++)
    {
        
        if(total + linesize < limit)
        {
            memcpy(split, src, TEXTURE_BLOCK_SIZE * 4);

        }
        src += linesize;
        total += linesize;
        split += (TEXTURE_BLOCK_SIZE * 4);
    }

    return out;
}
