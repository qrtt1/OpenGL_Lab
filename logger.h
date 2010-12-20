#ifndef __LOGGER_H
#define __LOGGER_H

#define ENABLE_LOGGING 1
#ifdef ENABLE_LOGGING
#include<stdio.h>
#endif

#ifdef ENABLE_LOGGING
//#define logI(tag, fmt, ...) do{printf(fmt, ## __VA_ARGS__);}while(0)
#define logI(tag, fmt, ...)
#else
#define logI(tag, fmt, ...)
#endif


#ifdef ENABLE_LOGGING
#define logD(tag, fmt, ...) do{printf(fmt, ## __VA_ARGS__);printf("\n");}while(0)
//#define logD(tag, fmt, ...)
#else
#define logD(tag, fmt, ...)
#endif


#endif // __LOGGER_H
