//
//  Util.m
//  Lab2
//
//  Created by Apple on 2010/12/21.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "Util.h"


@implementation Util
+(double)getTime
{
    double _t = 0;
    NSDate *today = [NSDate date];
    _t = [today timeIntervalSince1970];
    return _t;
}
@end
