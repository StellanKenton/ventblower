/************************************************************************************
* @file     : main.c
* @brief    : Application entry point.
* @details  : Initializes log, console, BSP and worker tasks, then starts the RTOS scheduler.
* @author   : Stellan
* @date     :
* @version  :
* @copyright: Copyright (c) 2026 ResMed. All rights reserved.
***********************************************************************************/
#include "console.h"
#include "log.h"
#include "rtos.h"
#include "sysdebug.h"
#include "taskmanager.h"
#include "adc.h"
#include "bspdebug.h"
#include "drvCurrent.h"
#include "drvCurrentDebug.h"
#include "hall.h"
#include "pwm.h"

static const char *const gMainTag = "main";

int main(void)
{
    logInit();
#if LOG_CONSOLE_ENABLE
    if (sysdebugConsoleRegister() != CONSOLE_STATUS_OK) {
        LOG_W(gMainTag, "sysdebug console registration failed");
    }
    if (bspDebugConsoleRegister() != CONSOLE_STATUS_OK) {
        LOG_W(gMainTag, "bspdebug console registration failed");
    }
    if (drvCurrentDebugConsoleRegister() != CONSOLE_STATUS_OK) {
        LOG_W(gMainTag, "drvCurrent console registration failed");
    }
#endif

    LOG_R("*****************************************");
    LOG_R("             system boot");
    LOG_R("*****************************************");
    /* Bsp initialization: ADC first, TIMER0 in pwmInit() starts current sampling. */
    adcInit();
    drvCurrentInit();
    pwmInit();
    hallInit();

    /* Never start a partially wired system; the failure is logged by the task manager. */
    if (WorkerTasksRegister()) {
        /* Only returns on failure. */
        int8_t lSchedulerStatus = repRtosSchedulerStart();
        LOG_E(gMainTag, "scheduler start failed: %d", (int)lSchedulerStatus);
    }

    /* Fail-safe trap: PWM outputs stay disabled (POEN off). */
    for (;;) {
    }
}

/**************************End of file********************************/
