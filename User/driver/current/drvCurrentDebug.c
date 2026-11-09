/************************************************************************************
* @file     : drvCurrentDebug.c
* @brief    : Phase current monitor console command.
* @details  : cur shows the monitor state, cur cal recalibrates and clears a fault.
* @author   : Stellan
* @date     :
* @version  :
* @copyright: Copyright (c) 2026 ResMed. All rights reserved.
***********************************************************************************/
#include "drvCurrentDebug.h"

#include <string.h>

#include "console.h"
#include "drvCurrent.h"
#include "log.h"

static const char *const gDrvCurrentDebugTag = "cur";
/* Indexed by eDrvCurrentState. */
static const char *const gDrvCurrentDebugStateName[] = {"calibrating", "ready", "cal_failed", "overcurrent"};

static int drvCurrentDebugToMa(float amps) {
    return (int)(amps * DRV_CURRENT_DEBUG_MA_PER_A);
}

static void drvCurrentDebugShow(void) {
    stDrvCurrentData lData;

    (void)drvCurrentRead(&lData);
    LOG_I(gDrvCurrentDebugTag, "state %s", gDrvCurrentDebugStateName[lData.state]);
    LOG_I(gDrvCurrentDebugTag, "now mA iu %d iv %d iw %d",
          drvCurrentDebugToMa(lData.current[ADC_CH_IU]),
          drvCurrentDebugToMa(lData.current[ADC_CH_IV]),
          drvCurrentDebugToMa(lData.current[ADC_CH_IW]));
    LOG_I(gDrvCurrentDebugTag, "peak mA iu %d iv %d iw %d",
          drvCurrentDebugToMa(lData.peak[ADC_CH_IU]),
          drvCurrentDebugToMa(lData.peak[ADC_CH_IV]),
          drvCurrentDebugToMa(lData.peak[ADC_CH_IW]));
    LOG_I(gDrvCurrentDebugTag, "offset counts iu %d iv %d iw %d",
          (int)lData.offset[ADC_CH_IU],
          (int)lData.offset[ADC_CH_IV],
          (int)lData.offset[ADC_CH_IW]);
}

static int8_t drvCurrentDebugCommand(const char *arguments) {
    if (arguments[0] == '\0') {
        drvCurrentDebugShow();
        return CONSOLE_STATUS_OK;
    }
    if (strcmp(arguments, "cal") != 0) {
        LOG_I(gDrvCurrentDebugTag, "usage: cur [cal]");
        return CONSOLE_STATUS_INVALID_ARGUMENT;
    }
    if (drvCurrentCalibrateStart() != DRV_CURRENT_STATUS_OK) {
        LOG_W(gDrvCurrentDebugTag, "calibration already running");
        return CONSOLE_STATUS_ERROR;
    }

    LOG_I(gDrvCurrentDebugTag, "calibration started, gates off");
    return CONSOLE_STATUS_OK;
}

static const stConsoleCommand gDrvCurrentDebugCommand = {
    "cur",
    "Phase current monitor: cur [cal]",
    drvCurrentDebugCommand
};

int8_t drvCurrentDebugConsoleRegister(void) {
    return consoleRegisterCommand(&gDrvCurrentDebugCommand);
}

/**************************End of file********************************/
