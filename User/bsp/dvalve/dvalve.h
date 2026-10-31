/************************************************************************************
* @file     : dvalve.h
* @brief    : Board proportional valve PWM interface.
* @details  : Selects a valve by index and controls its duty cycle in percent.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef USER_BSP_DVALVE_H
#define USER_BSP_DVALVE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DVALVE_STATUS_OK                 ((int8_t)1)
#define DVALVE_ERROR_INVALID_INDEX       ((int8_t)-1)
#define DVALVE_ERROR_INVALID_DUTY        ((int8_t)-2)
#define DVALVE_DUTY_MAX_PERCENT          100U

typedef enum eDvalveIndex {
    DVALVE_IDX_O2 = 0,
    DVALVE_IDX_RELIEF,
    DVALVE_IDX_EXP,
    DVALVE_COUNT
} eDvalveIndex;

void dvalveInit(void);
int8_t dvalveDutySet(eDvalveIndex valveIndex, uint8_t dutyPercent);

#ifdef __cplusplus
}
#endif

#endif /* USER_BSP_DVALVE_H */
/**************************End of file********************************/
