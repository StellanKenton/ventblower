/************************************************************************************
* @file     : sysdebug.c
* @brief    : System debug console command implementation.
* @details  : Provides MCU reboot and run-time query commands.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "sysdebug.h"

#include "console.h"
#include "gd32f4xx.h"
#include "log.h"
#include "portLog.h"

static const char *const gSysdebugTag = "sysdebug";

static eConsoleCommandResult sysdebugConsoleReboot(const char *arguments)
{
    uint32_t lDelayTick = 100000U;
    (void)arguments;
    LOG_T(gSysdebugTag, "Received command rebooting mcu");
    LOG_R(" ");
    LOG_R(" ");
    while (lDelayTick > 0U) {
        lDelayTick--;
    }
    NVIC_SystemReset();
    return CONSOLE_COMMAND_RESULT_OK;
}

static eConsoleCommandResult sysdebugConsoleTime(const char *arguments)
{
    (void)arguments;
    LOG_I(gSysdebugTag, "runtime %lu ms", (unsigned long)portLogGetRunTimeMs());

    return CONSOLE_COMMAND_RESULT_OK;
}

static eConsoleCommandResult sysdebugConsoleHelp(const char *arguments)
{
    (void)arguments;
    consoleShowHelp();

    return CONSOLE_COMMAND_RESULT_OK;
}

static const stConsoleCommand gSysdebugConsoleCommands[] = {
    {"reboot", "Restart the MCU", sysdebugConsoleReboot},
    {"time", "Show system runtime in milliseconds", sysdebugConsoleTime},
    {"help", "Show all registered commands", sysdebugConsoleHelp},
};

bool sysdebugConsoleRegister(void)
{
    uint32_t lIndex;

    for (lIndex = 0U; lIndex < (sizeof(gSysdebugConsoleCommands) / sizeof(gSysdebugConsoleCommands[0])); lIndex++) {
        if (!consoleRegisterCommand(&gSysdebugConsoleCommands[lIndex])) {
            return false;
        }
    }

    return true;
}
/**************************End of file********************************/
