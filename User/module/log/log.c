/************************************************************************************
* @file     : log.c
* @brief    : Lightweight logging core.
* @details  : Formats each line once on the caller stack and writes it to every enabled link.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "log.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

_Static_assert((LOG_LINE_MAX_SIZE >= 32U) && (LOG_RAW_LINE_MAX_SIZE >= 32U), "log line buffers are too small");

static const char *const gLogTag = "log";
/* Diagnostic counters only: an occasional lost increment between tasks is acceptable. */
static uint32_t gLogDropCount[LOG_LINK_COUNT];
static uint32_t gLogIsrDropCount = 0U;

static const char *logLevelToText(uint8_t level)
{
    switch (level) {
        case LOG_LEVEL_ERROR:
            return "E";
        case LOG_LEVEL_WARN:
            return "W";
        case LOG_LEVEL_INFO:
            return "I";
        case LOG_LEVEL_DEBUG:
            return "D";
        default:
            return "N";
    }
}

/** Clamp a printf-family return value to the range 0..maxLength. */
static uint32_t logClampLength(int length, uint32_t maxLength)
{
    if (length < 0) {
        return 0U;
    }

    return ((uint32_t)length < maxLength) ? (uint32_t)length : maxLength;
}

/**
 * Format one line into the caller buffer and write it to every enabled link.
 * The buffer lives on the caller stack, so formatting needs no lock and each link keeps its own
 * write atomic. A NULL tag selects a raw line without prefix.
 */
static void logOutput(char *line, uint32_t size, uint8_t level, const char *tag, const char *format, va_list args)
{
    const uint32_t lMaxLength = size - 2U;
    const stLogPortOps *lOps;
    uint32_t lLength = 0U;
    uint8_t lLinkId;

    /* Links may use RTOS critical sections, so they must never run inside an ISR. */
    if (logPortIsInIsr()) {
        gLogIsrDropCount++;
        return;
    }

    if (tag != NULL) {
        lLength = logClampLength(snprintf(line, lMaxLength + 1U, "[%s][%lu][%s] ", logLevelToText(level),
                                          (unsigned long)logPortGetTimeMs(), tag), lMaxLength);
    }
    lLength += logClampLength(vsnprintf(&line[lLength], (lMaxLength + 1U) - lLength,
                                        (format != NULL) ? format : "", args), lMaxLength - lLength);
    line[lLength++] = '\r';
    line[lLength++] = '\n';

    for (lLinkId = 0U; lLinkId < LOG_LINK_COUNT; lLinkId++) {
        lOps = logPortGetOps(lLinkId);
        if ((lOps != NULL) && (lOps->write != NULL) &&
            (lOps->write((const uint8_t *)line, lLength) != lLength)) {
            gLogDropCount[lLinkId]++;
        }
    }
}

void logInit(void)
{
    const stLogPortOps *lOps;
    uint8_t lLinkId;

    for (lLinkId = 0U; lLinkId < LOG_LINK_COUNT; lLinkId++) {
        lOps = logPortGetOps(lLinkId);
        if ((lOps != NULL) && (lOps->init != NULL)) {
            lOps->init();
        }
    }
}

void logWrite(uint8_t level, const char *tag, const char *format, ...)
{
    char lLine[LOG_LINE_MAX_SIZE];
    va_list lArgs;

    va_start(lArgs, format);
    logOutput(lLine, (uint32_t)sizeof(lLine), level, (tag != NULL) ? tag : "", format, lArgs);
    va_end(lArgs);
}

void logRawWrite(const char *format, ...)
{
    char lLine[LOG_RAW_LINE_MAX_SIZE];
    va_list lArgs;

    va_start(lArgs, format);
    logOutput(lLine, (uint32_t)sizeof(lLine), LOG_LEVEL_NONE, NULL, format, lArgs);
    va_end(lArgs);
}

void logShowStats(void)
{
    const stLogPortOps *lOps;
    uint8_t lLinkId;

    /* Requested output, so it bypasses the compile-time level filter. */
    for (lLinkId = 0U; lLinkId < LOG_LINK_COUNT; lLinkId++) {
        lOps = logPortGetOps(lLinkId);
        if (lOps != NULL) {
            logWrite(LOG_LEVEL_INFO, gLogTag, "link %s dropped %lu", (lOps->name != NULL) ? lOps->name : "?",
                     (unsigned long)gLogDropCount[lLinkId]);
        }
    }
    logWrite(LOG_LEVEL_INFO, gLogTag, "isr dropped %lu", (unsigned long)gLogIsrDropCount);
}

/**************************End of file********************************/
