//
//  Square.m
//  Lab2
//
//  Created by Apple on 2010/12/18.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "Shape.h"
#include "ffmpeg_context.h"

static BOOL getPix = FALSE;
static int tw = 0, th = 0;
static GLuint textureId;


@implementation Shape
-(id)init
{
    if(self = [super init])
    {
        if (!getPix) {
            init_ffmpeg_context();
            FFmpegContext _ctx, *ctx = &_ctx;
            RESET_CONTEXT_FOR_OPEN(ctx);
            //const char *source = "mmst://203.69.41.22/NA_IMTV_ONLINETV-1_450K_PC";
            //const char* source = "mms://210.59.147.3/wmtencoder/100k.wmv";
            
            const char *source = "mmsh://wmslive.media.hinet.net/bc000097";
            openMediaSource(ctx, source, NULL);
            getMediaInfo(ctx);
            
            
            getTextureLayout(ctx, &tw, &th);
            NSLog(@"texture block %dx%d", tw, th);
            
            
            
            while (IS_READY_TO_USE(ctx)) {
                nextFrame(ctx);
                if (ctx->result.type == RESULT_TYPE_VIDEO) {
                    
                    uint8_t *split = NULL;
                    /*
                    int width = ctx->videoCtx->width;
                    int height = ctx->videoCtx->height;
                    */
                    split = getTexture(ctx->result.data, ctx->result.bytes, ctx->videoCtx->width * 4, 0, 0);
                    if(split)
                    {
                        //memset(split, 0, 64 * 64 * 4);
                        glGenTextures(1, &textureId);
                        glBindTexture(GL_TEXTURE_2D, textureId);
                        
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                        
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
                        
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, split);
                        //sprintf(tmpfile, "split_%d_%d.raw", w, h);
                        //FILE *abc = fopen(tmpfile, "wb");
                        //fwrite(split, sizeof(uint8_t), 64 * 64 * 4, abc);
                        //fclose(abc);
                        free(split);
                        NSLog(@"create texture %d", textureId);
                    }
                    
                    
                    break ;
                }
            }
            
            closeMediaSource(ctx); 
            getPix = TRUE;
        }

    }
    return self;
}

-(void)render
{
    
    //[self render_old];
    
    GLfixed x = 20, y = 30;
    
    const GLfloat squareVertices[] = {
        0.0f, 0.0f,
        0.0f, 64.0f,
        64.0f, 0.0f,
        64.0f, 64.0f
    };
    
    /*
     
     const GLfloat squareVertices[] = {
     -0.5f, -0.5f,
     0.5f, -0.5f,
     -0.5f,  0.5f,
     0.5f,  0.5f,
     };

     */
     
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glDisableClientState(GL_COLOR_ARRAY);
    
    const GLfloat texCoords[] = {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f
    };
    
    const GLshort spriteTexcoords[] = {
        0, 0,
        1, 0,
        0, 1,
        1, 1,
    };
    
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glPushMatrix();
    //glTranslatex(6.0f, 9.0f, 0.0f);
//    glRotatef(0,0f, 0,0f, 0,0f, 1,0f);
    glRotatef(0.0f, 0.0f, 0.0f, 1.0f);
    //glScalef(0.5f, 0.5f, 1.0f);
    
    /* 我省略了 scale 跟 rotate */
    
    glVertexPointer(2, GL_FLOAT, 0, squareVertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    
    glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
//    glTexCoordPointer(2, GL_SHORT, 0, spriteTexcoords);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_TEXTURE_2D);
    
    glPopMatrix();
    
}

-(void)render_old
{
	
    static const GLfloat squareVertices[] = {
        -0.5f,  -0.33f,
		0.5f,  -0.33f,
        -0.5f,   0.33f,
		0.5f,   0.33f,
    };
	
    static const GLubyte squareColors[] = {
        255, 255,   0, 255,
        0,   255, 255, 255,
        0,     0,   0,   0,
        255,   0, 255, 255,
    };
	
	static float transY = 0.0f;
	
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
   // glTranslatef(0.0f, (GLfloat)(sinf(transY)/2.0f), 0.0f);
   // transY += 0.075f;
	
    //glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    //glClear(GL_COLOR_BUFFER_BIT);
	
    glVertexPointer(2, GL_FLOAT, 0, squareVertices);
    glEnableClientState(GL_VERTEX_ARRAY);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, squareColors);
    glEnableClientState(GL_COLOR_ARRAY);
	
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	
}
@end
