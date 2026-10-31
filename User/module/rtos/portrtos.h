/************************************************************************************
* @file     : portrtos.h
* @brief    : FreeRTOS provider for the project RTOS abstraction.
* @details  : Exposes the FreeRTOS operations table.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef USER_MODULE_PORT_RTOS_H
#define USER_MODULE_PORT_RTOS_H

#include "rtos.h"

#ifdef __cplusplus
extern "C" {
#endif

const stRepRtosOps *portRtosGetOps(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_MODULE_PORT_RTOS_H */
/**************************End of file********************************/
