//
//  ScreenMode.h
//  Lab2
//
//  Created by Apple on 2010/12/21.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>

@interface ScreenMode : NSObject {
    GLfloat x;
    GLfloat y;
    
    GLfloat left;
    GLfloat right;
    GLfloat bottom;
    GLfloat top;
    GLfloat near;
    GLfloat far;
}

@property GLfloat x;
@property GLfloat y;

@property GLfloat left;
@property GLfloat right;
@property GLfloat bottom;
@property GLfloat top;
@property GLfloat near;
@property GLfloat far;


+(ScreenMode*) fitScreenWithWidth: (GLfloat)width height:(GLfloat)height;
+(ScreenMode*) fitVideoWithWidth: (GLfloat)width height:(GLfloat)height;


@end
