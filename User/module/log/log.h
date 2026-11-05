/************************************************************************************
* @file     : log.h
* @brief    : Lightweight logging interface.
* @details  : Provides LOG_E/LOG_W/LOG_I/LOG_D/LOG_R and the link contract implemented by portlog.c.
* @author   : Stellan
* @date     : 2026-11-04
* @version  : V2.0.0
* @copyright: Copyright (c) 2026 ResMed. All rights reserved.
***********************************************************************************/
#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stdint.h>

#include "portlog.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Levels are macros so that LOG_COMPILED_LEVEL can be compared in #if. */
#define LOG_LEVEL_NONE      0U
#define LOG_LEVEL_ERROR     1U
#define LOG_LEVEL_WARN      2U
#define LOG_LEVEL_INFO      3U
#define LOG_LEVEL_DEBUG     4U

typedef void (*pfLogPortInit)(void);
typedef uint32_t (*pfLogPortWrite)(const uint8_t *data, uint32_t length);
typedef uint32_t (*pfLogPortRead)(uint8_t *data, uint32_t length);
typedef uint32_t (*pfLogPortPending)(void);

/*
 * One output/input link, see log.md for the full contract.
 * write is mandatory: it never blocks and takes a whole line (returns length) or nothing (returns 0).
 * init, read and pending are optional. Links are only called from tasks or before the scheduler
 * starts, because the core drops calls made from an ISR.
 */
typedef struct stLogPortOps {
    const char *name;
    pfLogPortInit init;
    pfLogPortWrite write;
    pfLogPortRead read;
    pfLogPortPending pending;
} stLogPortOps;

/* Port hooks implemented by portlog.c. */
const stLogPortOps *logPortGetOps(uint8_t linkId);
uint32_t logPortGetTimeMs(void);
bool logPortIsInIsr(void);

/** Initialize every enabled link; call once at the start of main(). */
void logInit(void);

/** Write one "[L][ms][tag] text" line to every enabled link. Task or pre-scheduler context. */
void logWrite(uint8_t level, const char *tag, const char *format, ...) __attribute__((format(printf, 3, 4)));

/** Write one line without prefix or level filtering, e.g. test data records. */
void logRawWrite(const char *format, ...) __attribute__((format(printf, 1, 2)));

/** Print the dropped line count of every enabled link and the dropped ISR calls. */
void logShowStats(void);

#define LOG_R(format, ...) logRawWrite(format, ##__VA_ARGS__)

#if LOG_COMPILED_LEVEL >= LOG_LEVEL_ERROR
#define LOG_E(tag, format, ...) logWrite(LOG_LEVEL_ERROR, tag, format, ##__VA_ARGS__)
#else
#define LOG_E(tag, format, ...) ((void)0)
#endif

#if LOG_COMPILED_LEVEL >= LOG_LEVEL_WARN
#define LOG_W(tag, format, ...) logWrite(LOG_LEVEL_WARN, tag, format, ##__VA_ARGS__)
#else
#define LOG_W(tag, format, ...) ((void)0)
#endif

#if LOG_COMPILED_LEVEL >= LOG_LEVEL_INFO
#define LOG_I(tag, format, ...) logWrite(LOG_LEVEL_INFO, tag, format, ##__VA_ARGS__)
#else
#define LOG_I(tag, format, ...) ((void)0)
#endif

#if LOG_COMPILED_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_D(tag, format, ...) logWrite(LOG_LEVEL_DEBUG, tag, format, ##__VA_ARGS__)
#else
#define LOG_D(tag, format, ...) ((void)0)
#endif

#ifdef __cplusplus
}
#endif
#endif
/**************************End of file********************************/
