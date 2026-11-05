/************************************************************************************
* @file     : sysdebug.c
* @brief    : System debug console command implementation.
* @details  : Provides MCU reboot, run-time, log link statistics, task stack and help commands.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "sysdebug.h"

#include "console.h"
#include "gd32f4xx.h"
#include "log.h"
#include "rtos.h"
#include "taskmanager.h"

static const char *const gSysdebugTag = "sysdebug";

static int8_t sysdebugConsoleReboot(const char *arguments)
{
    (void)arguments;
    LOG_I(gSysdebugTag, "Received command rebooting mcu");
    LOG_R(" ");
    LOG_R(" ");
    /* The reset re-initializes the link buffers, so let the host read the last lines first. */
    (void)consoleDrain(0U, SYSDEBUG_REBOOT_DRAIN_TIMEOUT_MS);
    NVIC_SystemReset();
    return CONSOLE_STATUS_OK;
}

static int8_t sysdebugConsoleTime(const char *arguments)
{
    (void)arguments;
    LOG_I(gSysdebugTag, "runtime %lu ms", (unsigned long)repRtosGetTickMs());

    return CONSOLE_STATUS_OK;
}

static int8_t sysdebugConsoleLog(const char *arguments)
{
    (void)arguments;
    logShowStats();

    return CONSOLE_STATUS_OK;
}

static int8_t sysdebugConsoleTask(const char *arguments)
{
    (void)arguments;
    taskManagerShowStats();

    return CONSOLE_STATUS_OK;
}

static int8_t sysdebugConsoleHelp(const char *arguments)
{
    (void)arguments;
    consoleShowHelp();

    return CONSOLE_STATUS_OK;
}

static const stConsoleCommand gSysdebugConsoleCommands[] = {
    {"reboot", "Restart the MCU", sysdebugConsoleReboot},
    {"time", "Show system runtime in milliseconds", sysdebugConsoleTime},
    {"log", "Show dropped lines per log link", sysdebugConsoleLog},
    {"task", "Show task stack and heap low-water marks", sysdebugConsoleTask},
    {"help", "Show all registered commands", sysdebugConsoleHelp},
};

int8_t sysdebugConsoleRegister(void)
{
    uint32_t lIndex;
    int8_t lStatus;

    for (lIndex = 0U; lIndex < (sizeof(gSysdebugConsoleCommands) / sizeof(gSysdebugConsoleCommands[0])); lIndex++) {
        lStatus = consoleRegisterCommand(&gSysdebugConsoleCommands[lIndex]);
        if (lStatus != CONSOLE_STATUS_OK) {
            return lStatus;
        }
    }

    return CONSOLE_STATUS_OK;
}
/**************************End of file********************************/
