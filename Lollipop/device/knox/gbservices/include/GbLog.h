#ifndef GB_LOG_H
#define GB_LOG_H

#ifdef ANDROID

#include "android/log.h"

#define GLOGD(tag, ...) __android_log_print(ANDROID_LOG_DEBUG, tag, __VA_ARGS__)
#define GLOGE(tag, ...) __android_log_print(ANDROID_LOG_ERROR, tag, __VA_ARGS__)

#else

#include <stdio.h>
#define GLOGD(tag, ...) printf(tag, __VA_ARGS__)
#define GLOGE(tag, ...) printf(tag, __VA_ARGS__)

#endif


#endif // GB_LOG_H