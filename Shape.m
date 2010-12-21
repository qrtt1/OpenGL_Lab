//
//  Square.m
//  Lab2
//
//  Created by Apple on 2010/12/18.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "Shape.h"
#include "ffmpeg_context.h"
#include "ImageTexture.h"
#import "Util.h"

static BOOL getPix = FALSE;

FFmpegContext _ctx, *ctx = &_ctx;

static double startTime = -0.1f;
static long renderCount = 0;


@implementation Shape
-(id)init
{
    if(self = [super init])
    {
        if (!getPix) {
            init_ffmpeg_context();
            
            RESET_CONTEXT_FOR_OPEN(ctx);
            //const char *source = "mmst://203.69.41.22/NA_IMTV_ONLINETV-1_450K_PC";
            //const char* source = "mms://210.59.147.3/wmtencoder/100k.wmv";
            
            const char *source = "mmsh://wmslive.media.hinet.net/bc000097";
            openMediaSource(ctx, source, NULL);
            getMediaInfo(ctx);
                         
            
            
//            closeMediaSource(ctx); 
            getPix = TRUE;
        }
        
        if (startTime < 0)
        {
            startTime = [Util getTime];   
        }

    }
    return self;
}

-(void)renderSplitTextureAtCol:(GLfixed)c row:(GLfixed)r textures:(ImageTexture*)imageTexture
{
    
    GLfixed x = c * 64;
    GLfixed y = r * 64;
    
    const GLfloat squareVertices[] = {
        0.0f + x, 0.0f + y,
        0.0f + x, 64.0f + y,
        64.0f + x, 0.0f + y,
        64.0f + x, 64.0f + y
    };
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glDisableClientState(GL_COLOR_ARRAY);
    
    const GLfloat texCoords[] = {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f
    };
    
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, [imageTexture getTextureAtX:c Y:r]);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPushMatrix();
    
    /*
    glTranslatef(640.0f + 64.0f , 780.0f, 0);
    glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
    glScalef(2.0f, 2.0f, 1.0f);
    */
     
    glVertexPointer(2, GL_FLOAT, 0, squareVertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    
    glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_TEXTURE_2D);
    
    glPopMatrix();
}



-(void)render
{
    ImageTexture *imageTexture = nil;
    while (IS_READY_TO_USE(ctx)) {
        nextFrame(ctx);
        if (!IS_READY_TO_USE(ctx)) {
            return ;
        }
        if (ctx->result.type == RESULT_TYPE_VIDEO) {
            //NSLog(@"create new imageTexture");
            imageTexture = [[ImageTexture alloc] initWithImageData:ctx->result.data limit:ctx->result.bytes ctx:ctx aPointBytes:4];
            break ;
        }
    }
    
    int c = [imageTexture getColumn];
    int r = [imageTexture getRow];
    
    for (int j=0; j<r; j++) {
        for (int i=0; i<c; i++) {
            [self renderSplitTextureAtCol:i row:j textures: imageTexture];
        }
    }
    
    angle += 5.0f;
    
    [imageTexture dealloc];
    
    
    renderCount ++;
    double timediff = [Util getTime] - startTime;
    if (renderCount) {
        NSLog(@"fps: %f",  renderCount / timediff);
    }
    
}


-(void)dealloc
{
    [super dealloc];
}
@end
