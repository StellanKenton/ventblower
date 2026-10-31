/************************************************************************************
* @file     : console.c
* @brief    : RTT console command processing.
* @details  : Reads RTT down-buffer input and dispatches complete command lines.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "console.h"

#include <stddef.h>

#include "log.h"
#include "SEGGER_RTT.h"

static const char *const gConsoleTag = "console";
static char gConsoleCommandBuffer[CONSOLE_COMMAND_MAX_LENGTH];
static uint32_t gConsoleCommandLength = 0U;
static const stConsoleCommand *gConsoleRegisteredCommands[CONSOLE_REGISTERED_COMMAND_MAX];
static uint32_t gConsoleRegisteredCommandCount = 0U;

static bool consoleIsSpace(char value)
{
    return (value == ' ') || (value == '\t');
}

static bool consoleCommandNameEqual(const char *command, const char *name)
{
    uint32_t lIndex = 0U;

    if ((command == NULL) || (name == NULL)) {
        return false;
    }

    while ((command[lIndex] != '\0') && (name[lIndex] != '\0')) {
        if (command[lIndex] != name[lIndex]) {
            return false;
        }
        lIndex++;
    }

    return (name[lIndex] == '\0') &&
           ((command[lIndex] == '\0') || consoleIsSpace(command[lIndex]));
}

static eConsoleCommandResult consoleRunCommand(const stConsoleCommand *entry, char *command)
{
    char *lArguments = command;

    while ((*lArguments != '\0') && !consoleIsSpace(*lArguments)) {
        lArguments++;
    }
    while (consoleIsSpace(*lArguments)) {
        lArguments++;
    }

    return entry->handler(lArguments);
}

static char *consoleTrimCommand(char *command)
{
    char *lStart = command;
    char *lEnd;

    if (command == NULL) {
        return NULL;
    }

    while (consoleIsSpace(*lStart)) {
        lStart++;
    }

    lEnd = lStart;
    while (*lEnd != '\0') {
        lEnd++;
    }
    while ((lEnd > lStart) && consoleIsSpace(*(lEnd - 1))) {
        lEnd--;
    }
    *lEnd = '\0';

    return lStart;
}

static void consoleDispatchCommand(char *command)
{
    uint32_t lIndex;
    char *lCommand = consoleTrimCommand(command);

    if ((lCommand == NULL) || (lCommand[0] == '\0')) {
        return;
    }

    for (lIndex = 0U; lIndex < gConsoleRegisteredCommandCount; lIndex++) {
        if (consoleCommandNameEqual(lCommand, gConsoleRegisteredCommands[lIndex]->name)) {
            if (consoleRunCommand(gConsoleRegisteredCommands[lIndex], lCommand) != CONSOLE_COMMAND_RESULT_OK) {
                LOG_W(gConsoleTag, "command failed: %s", lCommand);
            }
            return;
        }
    }

    LOG_W(gConsoleTag, "unknown command: %s", lCommand);
}

bool consoleRegisterCommand(const stConsoleCommand *command)
{
    uint32_t lIndex;

    if ((command == NULL) || (command->name == NULL) || (command->name[0] == '\0') ||
        (command->description == NULL) || (command->handler == NULL)) {
        return false;
    }

    for (lIndex = 0U; lIndex < gConsoleRegisteredCommandCount; lIndex++) {
        if (gConsoleRegisteredCommands[lIndex] == command) {
            return true;
        }
        if (consoleCommandNameEqual(command->name, gConsoleRegisteredCommands[lIndex]->name)) {
            return false;
        }
    }

    if (gConsoleRegisteredCommandCount >= CONSOLE_REGISTERED_COMMAND_MAX) {
        return false;
    }

    gConsoleRegisteredCommands[gConsoleRegisteredCommandCount++] = command;
    return true;
}

void consoleShowHelp(void)
{
    uint32_t lIndex;

    LOG_I(gConsoleTag, "registered commands:");
    for (lIndex = 0U; lIndex < gConsoleRegisteredCommandCount; lIndex++) {
        LOG_I(gConsoleTag, "  %s - %s", gConsoleRegisteredCommands[lIndex]->name,
              gConsoleRegisteredCommands[lIndex]->description);
    }
}

static void consolePushChar(char value)
{
    if ((value == '\r') || (value == '\n')) {
        gConsoleCommandBuffer[gConsoleCommandLength] = '\0';
        consoleDispatchCommand(gConsoleCommandBuffer);
        gConsoleCommandLength = 0U;
        return;
    }

    if ((value == '\b') || (value == 0x7FU)) {
        if (gConsoleCommandLength > 0U) {
            gConsoleCommandLength--;
        }
        return;
    }

    if (gConsoleCommandLength >= (CONSOLE_COMMAND_MAX_LENGTH - 1U)) {
        gConsoleCommandLength = 0U;
        LOG_W(gConsoleTag, "command too long");
        return;
    }

    gConsoleCommandBuffer[gConsoleCommandLength++] = value;
}

bool consoleProcess(uint16_t TickMs)
{
    char lReadBuffer[CONSOLE_RTT_READ_BUFFER_SIZE];
    unsigned lReadLength;
    unsigned lIndex;

    (void)TickMs;

    do {
        lReadLength = SEGGER_RTT_Read(0U, lReadBuffer, (unsigned)sizeof(lReadBuffer));
        for (lIndex = 0U; lIndex < lReadLength; lIndex++) {
            consolePushChar(lReadBuffer[lIndex]);
        }
    } while (lReadLength == (unsigned)sizeof(lReadBuffer));

    return true;
}
/**************************End of file********************************/
