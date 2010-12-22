//
//  ES1Renderer.m
//  Lab2
//
//  Created by Apple on 2010/12/18.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "ES1Renderer.h"
#import "Drawable.h"
#import "Shape.h"
#import "ScreenMode.h"

@implementation ES1Renderer

// Create an OpenGL ES 1.1 context
- (id)init
{
    self = [super init];
    if (self)
    {
        context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];

        if (!context || ![EAGLContext setCurrentContext:context])
        {
            [self release];
            return nil;
        }

        // Create default framebuffer object. The backing will be allocated for the current layer in -resizeFromLayer
        glGenFramebuffersOES(1, &defaultFramebuffer);
        glGenRenderbuffersOES(1, &colorRenderbuffer);
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, defaultFramebuffer);
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, colorRenderbuffer);
        glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, colorRenderbuffer);
    
	
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
		glDisable(GL_DITHER);
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
		//glShade(GL_FLAT);
		glDisable(GL_DEPTH_TEST);
	}

	
	
    return self;
}

- (void)render
{
    // This application only creates a single context which is already set current at this point.
    // This call is redundant, but needed if dealing with multiple contexts.
    [EAGLContext setCurrentContext:context];

    // This application only creates a single default framebuffer which is already bound at this point.
    // This call is redundant, but needed if dealing with multiple framebuffers.
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, defaultFramebuffer);
    glViewport(0, 0, backingWidth, backingHeight);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    /* 將參數交給 ScreenMode 管理 */
    if(TRUE)
    {
        id sm = [ScreenMode fitVideoWithWidth:backingWidth height:backingHeight];
        glOrthof([sm left], [sm right], [sm bottom], [sm top], [sm near], [sm far]);
        [sm release];
        
    }
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
   // glTranslatef(0.0f, (GLfloat)(sinf(transY)/2.0f), 0.0f);
   // transY += 0.075f;

    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
 
	id <Drawable> s = [[Shape alloc] init];
	[s render];
    // This application only creates a single color renderbuffer which is already bound at this point.
    // This call is redundant, but needed if dealing with multiple renderbuffers.
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, colorRenderbuffer);
    [context presentRenderbuffer:GL_RENDERBUFFER_OES];
    
    [s release];
}

- (BOOL)resizeFromLayer:(CAEAGLLayer *)layer
{	
    // Allocate color buffer backing based on the current layer size
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, colorRenderbuffer);
    [context renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:layer];
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &backingWidth);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &backingHeight);

    if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES)
    {
        NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
        return NO;
    }

    return YES;
}

- (void)dealloc
{
    // Tear down GL
    if (defaultFramebuffer)
    {
        glDeleteFramebuffersOES(1, &defaultFramebuffer);
        defaultFramebuffer = 0;
    }

    if (colorRenderbuffer)
    {
        glDeleteRenderbuffersOES(1, &colorRenderbuffer);
        colorRenderbuffer = 0;
    }

    // Tear down context
    if ([EAGLContext currentContext] == context)
        [EAGLContext setCurrentContext:nil];
    
    [context release];
    context = nil;

    [super dealloc];
}

@end
