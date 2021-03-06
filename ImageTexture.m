//
//  ImageTexture.m
//  Lab2
//
//  Created by Apple on 2010/12/20.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "ImageTexture.h"
#include "image_spliter.h"


int tw = 0, th = 0;

@implementation ImageTexture
-(id) initWithImageData: (uint8_t*)data limit:(int)length ctx:(FFmpegContext*)c aPointBytes:(int) bytes
{
        
    textures = NULL;
    if(self = [super init])
    {
        getTextureLayout(c, &tw, &th);
        textureLen = tw * th;
        textures = malloc(sizeof(GLuint) * textureLen);
        
        uint8_t *split = NULL;
        int cnt = 0;
        for (int j=0; j<th; j++) {
            for (int i=0; i<tw; i++) {
                split = getTexture(data, length, c->videoCtx->width * bytes, i, j);
                if(split)
                {
                    glGenTextures(1, &textures[cnt]);
                    glBindTexture(GL_TEXTURE_2D, textures[cnt]);
                    //
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    //
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
                    //
                    
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TEXTURE_BLOCK_SIZE, TEXTURE_BLOCK_SIZE, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, split);
                    cnt++;
                    free(split);
                }
                split = NULL;
            }
        }
    }
    
    return self;
}

-(GLuint*)getTextures
{
    return textures;
}

-(GLuint)getTextureAtX:(int)x Y:(int)y
{
    return textures[y * tw + x];
}

-(int)getRow
{
    return th;
}

-(int)getColumn
{
    return tw;
}

-(void)dealloc
{
    
    for (int i=0; i<textureLen; i++) {
        glDeleteTextures(1, &textures[i]);
    }

    if (textures != NULL) {
        free(textures);
    }
    
    [super dealloc];

}
@end
