//
//  ScreenMode.m
//  Lab2
//
//  Created by Apple on 2010/12/21.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "ScreenMode.h"


@implementation ScreenMode

@synthesize x;
@synthesize y;

@synthesize left;
@synthesize right;
@synthesize bottom;
@synthesize top;
@synthesize near;
@synthesize far;


+(ScreenMode*) fitScreenWithWidth: (GLfloat)width height:(GLfloat)height
{
    //NSLog(@"invoke verticalWidth");
    ScreenMode* m = [[ScreenMode alloc] init];
    m->x = 0.0f;
    m->y = 0.0f;
    
    m->left = 0.0f;
    m->right = height;
    m->bottom = width;
    m->top = 0.0f;
    m->near = -1.0f;
    m->far = 1.0f;

    return m;
}


+(ScreenMode*) fitVideoWithWidth: (GLfloat)width height:(GLfloat)height
{
    //NSLog(@"invoke verticalWidth");
    //glOrthof(backingWidth, 0.0f, 0.0f, backingHeight, -1.0f, 1.0f);

    ScreenMode* m = [[ScreenMode alloc] init];
    m->x = 0.0f;
    m->y = 0.0f;
    
    /*
    m->left = width;
    m->right = 0.0f;
    m->bottom = 0.0f;
    m->top = height;
    */
    
    // up 
    m->left = 0.0f;
    m->right = width;
    m->bottom = height;
    m->top = 0.0f;
    
    m->near = -1.0f;
    m->far = 1.0f;
    
    

    
    return m;
}


@end
