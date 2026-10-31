/************************************************************************************
* @file     : valve.h
* @brief    : Board zeroing valve GPIO interface.
* @details  : Defines valve indexes, control levels, and GPIO access functions.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef USER_BSP_VALVE_H
#define USER_BSP_VALVE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VALVE_STATUS_OK                 ((int8_t)1)
#define VALVE_ERROR_INVALID_INDEX       ((int8_t)-1)
#define VALVE_ERROR_INVALID_LEVEL       ((int8_t)-2)

typedef enum eValveIndex {
    VALVE_IDX_INSP_PRESSURE_ZERO = 0,
    VALVE_IDX_EXP_PRESSURE_ZERO,
    VALVE_IDX_DIFF_PRESSURE_ZERO,
    VALVE_IDX_FLUSH,
    VALVE_COUNT
} eValveIndex;

typedef enum eValveLevel {
    VALVE_LEVEL_INVALID = -1,
    VALVE_LEVEL_LOW = 0,
    VALVE_LEVEL_HIGH
} eValveLevel;

void valveInit(void);
int8_t valveControlSet(eValveIndex valveIndex, eValveLevel level);
eValveLevel valveStateGet(eValveIndex valveIndex);

#ifdef __cplusplus
}
#endif

#endif /* USER_BSP_VALVE_H */
/**************************End of file********************************/
