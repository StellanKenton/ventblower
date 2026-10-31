/************************************************************************************
* @file     : bspdebug.h
* @brief    : BSP debug console commands.
* @details  : Declares the BSP peripheral debug command registration entry.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef USER_BSP_BSPDEBUG_H
#define USER_BSP_BSPDEBUG_H

#include <stdbool.h>

#define BSP_DEBUG_BLOWER_SPEED_MAX_RPS        800U
#define BSP_DEBUG_BLOWER_PWM_MAX_PERCENT     100U
#define BSP_DEBUG_BLOWER_PWM_SCALE            10U

#ifdef __cplusplus
extern "C" {
#endif

bool bspDebugConsoleRegister(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_BSP_BSPDEBUG_H */
/**************************End of file********************************/
