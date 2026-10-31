/************************************************************************************
* @file     : log.h
* @brief    : Lightweight logging interface.
* @details  : Provides unified LOG_I/LOG_E/LOG_W/LOG_D macros and raw RTT output.
* @author   : \.rumi
* @date     : 2026-03-31
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "ringbuffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum eLogLevel {
    LOG_LEVEL_NONE = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4,
} eLogLevel;

#include "portLog.h"

#ifndef LOG_OUTPUT_MAX_FRAME_SIZE
#define LOG_OUTPUT_MAX_FRAME_SIZE LOG_OUTPUT_BUFFER_SIZE
#endif

void logWrite(eLogLevel level, const char *tag, const char *format, ...) __attribute__((format(printf, 3, 4)));
void logVWrite(eLogLevel level, const char *tag, const char *format, ...) __attribute__((format(printf, 3, 4)));
void logRawWrite(const char *format, ...) __attribute__((format(printf, 1, 2)));

#define LOG_T(tag, format, ...) logVWrite((eLogLevel)LOG_LEVEL_INFO, tag, format, ##__VA_ARGS__)
#define LOG_R(format, ...) logRawWrite(format, ##__VA_ARGS__)

#if LOG_COMPILED_LEVEL >= LOG_LEVEL_ERROR
#define LOG_E(tag, format, ...) logWrite((eLogLevel)LOG_LEVEL_ERROR, tag, format, ##__VA_ARGS__)
#else
#define LOG_E(tag, format, ...) ((void)0)
#endif

#if LOG_COMPILED_LEVEL >= LOG_LEVEL_WARN
#define LOG_W(tag, format, ...) logWrite((eLogLevel)LOG_LEVEL_WARN, tag, format, ##__VA_ARGS__)
#else
#define LOG_W(tag, format, ...) ((void)0)
#endif

#if LOG_COMPILED_LEVEL >= LOG_LEVEL_INFO
#define LOG_I(tag, format, ...) logWrite((eLogLevel)LOG_LEVEL_INFO, tag, format, ##__VA_ARGS__)
#else
#define LOG_I(tag, format, ...) ((void)0)
#endif

#if LOG_COMPILED_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_D(tag, format, ...) logWrite((eLogLevel)LOG_LEVEL_DEBUG, tag, format, ##__VA_ARGS__)
#else
#define LOG_D(tag, format, ...) ((void)0)
#endif


bool logInit(void);
bool logProcess(uint16_t TickMs);

#ifdef __cplusplus
}
#endif
#endif
/**************************End of file********************************/
