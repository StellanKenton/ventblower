/************************************************************************************
* @file     : main.c
* @brief    : Application entry point.
* @details  : Starts the task manager and the RTOS scheduler.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "console.h"
#include "log.h"
#include "rtos.h"
#include "sysdebug.h"
#include "taskmanager.h"
#include "adc.h"
#include "bspdebug.h"
#include "dvalve.h"
#include "hall.h"
#include "pwm.h"

int main(void)
{
    int8_t lSchedulerStatus;

    logInit();
#if LOG_CONSOLE_ENABLE
    if (sysdebugConsoleRegister() != CONSOLE_STATUS_OK) {
        LOG_W("main", "sysdebug console registration failed");
    }
    if (bspDebugConsoleRegister() != CONSOLE_STATUS_OK) {
        LOG_W("main", "bspdebug console registration failed");
    }
#endif

    LOG_R("*****************************************");
    LOG_R("             system boot");
    LOG_R("*****************************************");
    /* Bsp initialization */
    adcInit();
    pwmInit();
    hallInit();
    dvalveInit();

    /* Register project tasks before handing control to the scheduler. */
    if (!WorkerTasksRegister()) {
        LOG_E("main", "worker task registration failed");
    }

    lSchedulerStatus = repRtosSchedulerStart();
    if (lSchedulerStatus != REP_RTOS_STATUS_OK) {
        LOG_E("main", "scheduler start failed: %d", (int)lSchedulerStatus);
    }

    /* A running scheduler never returns to the application entry point. */
    while(1) {
        // do nothing, the scheduler is running
    }
}

/**************************End of file********************************/
