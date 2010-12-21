//
//  ImageTexture.h
//  Lab2
//
//  Created by Apple on 2010/12/20.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#include "ffmpeg_context.h"
#include "image_spliter.h"

@interface ImageTexture : NSObject {
    @private
    
    GLuint* textures;
    GLuint textureLen;

}

static GLfloat OriginX = 0.0f;
static GLfloat OriginY = 0.0f;

-(id) initWithImageData: (uint8_t*)data limit:(int)length ctx:(FFmpegContext*)c aPointBytes:(int) bytes;
-(GLuint)getTextureAtX:(int)x Y:(int)y;
-(int)getRow;
-(int)getColumn;

@end
