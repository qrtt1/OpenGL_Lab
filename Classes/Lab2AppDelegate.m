//
//  Lab2AppDelegate.m
//  Lab2
//
//  Created by Apple on 2010/12/18.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "Lab2AppDelegate.h"
#import "EAGLView.h"

@implementation Lab2AppDelegate

@synthesize window;
@synthesize glView;

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions   
{
    [glView startAnimation];
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    [glView stopAnimation];
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    [glView startAnimation];
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    [glView stopAnimation];
}

- (void)dealloc
{
    [window release];
    [glView release];

    [super dealloc];
}

@end
