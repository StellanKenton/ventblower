/************************************************************************************
* @file     : rtos.c
* @brief    : Project RTOS abstraction.
* @details  : Forwards project RTOS calls to the selected platform port.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "rtos.h"

#include <stddef.h>

#include "portrtos.h"

static const stRepRtosOps *repRtosGetOps(void)
{
    return portRtosGetOps();
}

int8_t repRtosTaskCreate(const stRepRtosTaskConfig *config)
{
    const stRepRtosOps *lOps = repRtosGetOps();

    if ((lOps == NULL) || (lOps->taskCreate == NULL)) {
        return REP_RTOS_STATUS_NOT_READY;
    }

    return lOps->taskCreate(config);
}

void repRtosTaskDelete(repRtosTaskHandle handle)
{
    const stRepRtosOps *lOps = repRtosGetOps();

    if ((lOps == NULL) || (lOps->taskDelete == NULL)) {
        return;
    }

    lOps->taskDelete(handle);
}

int8_t repRtosTaskDelayMs(uint32_t delayMs)
{
    const stRepRtosOps *lOps = repRtosGetOps();

    if ((lOps == NULL) || (lOps->taskDelayMs == NULL)) {
        return REP_RTOS_STATUS_NOT_READY;
    }

    return lOps->taskDelayMs(delayMs);
}

int8_t repRtosTaskDelayUntilMs(uint32_t *previousWakeMs, uint32_t periodMs)
{
    const stRepRtosOps *lOps = repRtosGetOps();

    if ((lOps == NULL) || (lOps->taskDelayUntilMs == NULL)) {
        return REP_RTOS_STATUS_NOT_READY;
    }

    return lOps->taskDelayUntilMs(previousWakeMs, periodMs);
}

uint32_t repRtosGetTickMs(void)
{
    const stRepRtosOps *lOps = repRtosGetOps();

    if ((lOps == NULL) || (lOps->getTickMs == NULL)) {
        return 0U;
    }

    return lOps->getTickMs();
}

void repRtosEnterCritical(void)
{
    const stRepRtosOps *lOps = repRtosGetOps();

    if ((lOps == NULL) || (lOps->enterCritical == NULL)) {
        return;
    }

    lOps->enterCritical();
}

void repRtosExitCritical(void)
{
    const stRepRtosOps *lOps = repRtosGetOps();

    if ((lOps == NULL) || (lOps->exitCritical == NULL)) {
        return;
    }

    lOps->exitCritical();
}

int8_t repRtosSchedulerStart(void)
{
    const stRepRtosOps *lOps = repRtosGetOps();

    if ((lOps == NULL) || (lOps->schedulerStart == NULL)) {
        return REP_RTOS_STATUS_NOT_READY;
    }

    return lOps->schedulerStart();
}

/**************************End of file********************************/
