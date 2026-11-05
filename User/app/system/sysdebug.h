/************************************************************************************
* @file     : sysdebug.h
* @brief    : System debug console commands.
* @details  : Declares the reboot, time, log, task and help console commands.
* @author   : Stellan
* @date     :
* @version  :
* @copyright: Copyright (c) 2026 ResMed. All rights reserved.
***********************************************************************************/
#ifndef SYSDEBUG_H
#define SYSDEBUG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Upper bound for sending the last console lines before a commanded reset. */
#define SYSDEBUG_REBOOT_DRAIN_TIMEOUT_MS    100U

/** Register the system commands; returns CONSOLE_STATUS_OK or the console error status. */
int8_t sysdebugConsoleRegister(void);

#ifdef __cplusplus
}
#endif

#endif
/**************************End of file********************************/
