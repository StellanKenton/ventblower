/************************************************************************************
* @file     : drvAiicPort.h
* @brief    : Project binding for software I2C.
* @details  :
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef DRV_AIIC_PORT_H
#define DRV_AIIC_PORT_H

#include <stdint.h>

#define DRV_AIIC_USE_RTOS                    1U
#define DRV_AIIC_MAX                         2U
#define DRV_AIIC_LOCK_WAIT_MS                5U
#define DRV_AIIC_DEFAULT_HALF_PERIOD_US      10U
#define DRV_AIIC_DEFAULT_RECOVERY_CLOCKS     9U

#include "drvAiic.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRV_AIIC_BUS_TCA9535            0U
#define DRV_AIIC_BUS_TOUCH              1U

const stDrvAiicPortOps *drvAiicPortGetOps(uint8_t busId);

#ifdef __cplusplus
}
#endif

#endif
/**************************End of file********************************/
