/************************************************************************************
* @file     : portLog.h
* @brief    : Logging port interface.
* @details  : Provides the MCU run-time source used by the log prefix.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef PORT_LOG_H
#define PORT_LOG_H

#include <stdbool.h>
#include <stdint.h>

#define LOG_USE_RTOS        1
#define LOG_CONSOLE_ENABLE  1

#ifndef LOG_COMPILED_LEVEL
#define LOG_COMPILED_LEVEL 4
#endif

#ifndef LOG_INPUT_BUFFER_SIZE
#define LOG_INPUT_BUFFER_SIZE  512U
#endif

#ifndef LOG_OUTPUT_BUFFER_SIZE
#define LOG_OUTPUT_BUFFER_SIZE 256U
#endif

#ifndef LOG_OUTPUT_QUEUE_SIZE
#define LOG_OUTPUT_QUEUE_SIZE  4096U
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t (*portLogGetRunTimeMsFunc)(void);

typedef struct stPortLogOps {
    portLogGetRunTimeMsFunc getRunTimeMs;
} stPortLogOps;

bool portLogRegisterOps(const stPortLogOps *ops);
const stPortLogOps *portLogGetOps(void);
uint32_t portLogGetRunTimeMs(void);

#ifdef __cplusplus
}
#endif

#endif
/**************************End of file********************************/
