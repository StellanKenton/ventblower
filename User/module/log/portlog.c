/************************************************************************************
* @file     : portLog.c
* @brief    : Logging port implementation.
* @details  : Selects the MCU run-time provider according to the log configuration.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "portLog.h"

#include <stddef.h>

#if LOG_USE_RTOS
#include "rtos.h"
#endif

#if LOG_USE_RTOS
static const stPortLogOps gPortLogRtosOps = {
    .getRunTimeMs = repRtosGetTickMs,
};
#else
static const stPortLogOps *gPortLogBoardOps = {
    .getRunTimeMs = NULL,
};
#endif

bool portLogRegisterOps(const stPortLogOps *ops)
{
#if LOG_USE_RTOS
    (void)ops;
    return true;
#else
    if ((ops == NULL) || (ops->getRunTimeMs == NULL)) {
        return false;
    }

    gPortLogBoardOps = ops;
    return true;
#endif
}

const stPortLogOps *portLogGetOps(void)
{
#if LOG_USE_RTOS
    return &gPortLogRtosOps;
#else
    return gPortLogBoardOps;
#endif
}

uint32_t portLogGetRunTimeMs(void)
{
    const stPortLogOps *lOps = portLogGetOps();

    if ((lOps == NULL) || (lOps->getRunTimeMs == NULL)) {
        return 0U;
    }

    return lOps->getRunTimeMs();
}

/**************************End of file********************************/
