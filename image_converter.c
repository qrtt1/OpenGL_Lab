#include "image_converter.h"

Yuv2RgbContext* openYuv2RgbContext(Yuv2RgbContext* ctr, int width, int height)
{
    ctr->converter = sws_getContext(width, height, PIX_FMT_YUV420P, 
                width, height, PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);
    
    ctr->rgbFrame = avcodec_alloc_frame();
    int size = avpicture_get_size(PIX_FMT_RGB32, width, height);
    ctr->rgbBuffer = av_malloc( sizeof(uint8_t) * size );
    ctr->rgbBufferSize = size;

    // bind buffer and frame
    avpicture_fill((AVPicture*) ctr->rgbFrame, 
            ctr->rgbBuffer, PIX_FMT_RGB32, width, height); 

    return ctr;
}

void closeYuv2RgbContext(Yuv2RgbContext* ctr)
{
    if (ctr == 0) 
        return ;

    av_free(ctr->rgbFrame);
    av_free(ctr->rgbBuffer);
    sws_freeContext(ctr->converter);
}


void yuv2rgb(Yuv2RgbContext context, AVFrame *src, int srcHeight) 
{

    sws_scale(context.converter, 
	    (const uint8_t* const*) 
	    &src->data, 
	    src->linesize, 
	    0, srcHeight,
	    context.rgbFrame->data, 
	    context.rgbFrame->linesize);
}
