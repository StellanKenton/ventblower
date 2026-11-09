/************************************************************************************
* @file     : drvCurrentDebug.h
* @brief    : Phase current monitor console command.
* @details  :
* @author   : Stellan
* @date     :
* @version  :
* @copyright: Copyright (c) 2026 ResMed. All rights reserved.
***********************************************************************************/
#ifndef DRV_CURRENT_DEBUG_H
#define DRV_CURRENT_DEBUG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Amps to milliamps for integer log output. */
#define DRV_CURRENT_DEBUG_MA_PER_A      1000.0f

/**
 * @brief Register the cur command with the console.
 * @return CONSOLE_STATUS_OK, or the console error status.
 */
int8_t drvCurrentDebugConsoleRegister(void);

#ifdef __cplusplus
}
#endif

#endif /* DRV_CURRENT_DEBUG_H */
/**************************End of file********************************/
