/************************************************************************************
* @file     : log.c
* @brief    : RTT backed lightweight logging implementation.
* @details  : Formats log records into a byte ring buffer and flushes them through RTT.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "log.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#if LOG_CONSOLE_ENABLE
#include "console.h"
#endif
#include "portLog.h"
#if LOG_USE_RTOS
#include "rtos.h"
#endif
#include "SEGGER_RTT.h"

static stRingBuffer gLogOutputQueue;
static uint8_t gLogOutputQueueStorage[LOG_OUTPUT_QUEUE_SIZE];
static char gLogFormatBuffer[LOG_OUTPUT_MAX_FRAME_SIZE];
static uint8_t gLogFlushBuffer[LOG_OUTPUT_BUFFER_SIZE];
static bool gLogReady = false;

static const char *logLevelToText(eLogLevel level)
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

static uint32_t logStringLength(const char *text, uint32_t maxLength)
{
    uint32_t lLength = 0U;

    if (text == NULL) {
        return 0U;
    }

    while ((lLength < maxLength) && (text[lLength] != '\0')) {
        lLength++;
    }

    return lLength;
}

static uint32_t logFormatLine(eLogLevel level, const char *tag, const char *format, va_list args)
{
    int lPrefixLength;
    int lBodyLength;
    uint32_t lWriteLength;
    uint32_t lMaxBodyLength;
    const char *lTag = (tag != NULL) ? tag : "";
    const char *lFormat = (format != NULL) ? format : "";

    lPrefixLength = snprintf(gLogFormatBuffer,
                             sizeof(gLogFormatBuffer),
                             "[%s][%lu][%s] ",
                             logLevelToText(level),
                             (unsigned long)portLogGetRunTimeMs(),
                             lTag);
    if (lPrefixLength < 0) {
        return 0U;
    }
    if ((uint32_t)lPrefixLength >= (uint32_t)sizeof(gLogFormatBuffer)) {
        lPrefixLength = (int)sizeof(gLogFormatBuffer) - 1;
    }

    lMaxBodyLength = (uint32_t)sizeof(gLogFormatBuffer) - (uint32_t)lPrefixLength;
    if (lMaxBodyLength > 2U) {
        lBodyLength = vsnprintf(&gLogFormatBuffer[lPrefixLength], lMaxBodyLength, lFormat, args);
        if (lBodyLength < 0) {
            return 0U;
        }
    }

    lWriteLength = logStringLength(gLogFormatBuffer, (uint32_t)sizeof(gLogFormatBuffer) - 2U);
    gLogFormatBuffer[lWriteLength++] = '\r';
    gLogFormatBuffer[lWriteLength++] = '\n';

    return lWriteLength;
}

bool logInit(void)
{
    if (gLogReady) {
        return true;
    }

    if (ringBufferInit(&gLogOutputQueue, gLogOutputQueueStorage, LOG_OUTPUT_QUEUE_SIZE) != RINGBUFFER_OK) {
        return false;
    }

    SEGGER_RTT_Init();
    (void)SEGGER_RTT_SetFlagsUpBuffer(0U, SEGGER_RTT_MODE_NO_BLOCK_TRIM);
    (void)SEGGER_RTT_SetFlagsDownBuffer(0U, SEGGER_RTT_MODE_NO_BLOCK_TRIM);

    gLogReady = true;
    return true;
}

bool logProcess(uint16_t TickMs)
{
    uint32_t lReadLength;

    (void)TickMs;

    if (!gLogReady) {
        return logInit();
    }

    lReadLength = ringBufferRead(&gLogOutputQueue, gLogFlushBuffer, (uint32_t)sizeof(gLogFlushBuffer));
    if (lReadLength != 0U) {
        (void)SEGGER_RTT_Write(0U, gLogFlushBuffer, (unsigned)lReadLength);
    }

#if LOG_CONSOLE_ENABLE
    (void)consoleProcess(TickMs);
#endif

    return true;
}

void logVWrite(eLogLevel level, const char *tag, const char *format, ...)
{
    uint32_t lWriteLength;
    va_list lArgs;

    if (!gLogReady) {
        (void)logInit();
    }

#if LOG_USE_RTOS
    repRtosEnterCritical();
#endif
    va_start(lArgs, format);
    lWriteLength = logFormatLine(level, tag, format, lArgs);
    va_end(lArgs);
    if (lWriteLength != 0U) {
        (void)SEGGER_RTT_Write(0U, gLogFormatBuffer, (unsigned)lWriteLength);
    }
#if LOG_USE_RTOS
    repRtosExitCritical();
#endif
}

void logWrite(eLogLevel level, const char *tag, const char *format, ...)
{
    uint32_t lWriteLength;
    va_list lArgs;

    if (!gLogReady) {
        (void)logInit();
    }

#if LOG_USE_RTOS
    repRtosEnterCritical();
#endif
    va_start(lArgs, format);
    lWriteLength = logFormatLine(level, tag, format, lArgs);
    va_end(lArgs);
    if (lWriteLength != 0U) {
        (void)ringBufferWriteOverwrite(&gLogOutputQueue, (const uint8_t *)gLogFormatBuffer, lWriteLength);
    }
#if LOG_USE_RTOS
    repRtosExitCritical();
#endif
}

void logRawWrite(const char *format, ...)
{
    int lWriteLength;
    uint32_t lTextLength;
    const uint32_t lMaxTextLength = (uint32_t)sizeof(gLogFormatBuffer) - 2U;
    const char *lFormat = (format != NULL) ? format : "";
    va_list lArgs;

    if (!gLogReady) {
        (void)logInit();
    }

#if LOG_USE_RTOS
    repRtosEnterCritical();
#endif
    va_start(lArgs, format);
    lWriteLength = vsnprintf(gLogFormatBuffer, lMaxTextLength + 1U, lFormat, lArgs);
    va_end(lArgs);
    if (lWriteLength >= 0) {
        lTextLength = (uint32_t)lWriteLength;
        if (lTextLength > lMaxTextLength) {
            lTextLength = lMaxTextLength;
        }
        gLogFormatBuffer[lTextLength++] = '\r';
        gLogFormatBuffer[lTextLength++] = '\n';
        (void)SEGGER_RTT_Write(0U, gLogFormatBuffer, (unsigned)lTextLength);
    }
#if LOG_USE_RTOS
    repRtosExitCritical();
#endif
}

/**************************End of file********************************/
