/************************************************************************************
* @file     : console.c
* @brief    : Console command processing.
* @details  : Reads the console link input and dispatches complete command lines.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "console.h"

#include <stddef.h>

#include "log.h"

static const char *const gConsoleTag = "console";
static char gConsoleCommandBuffer[CONSOLE_COMMAND_MAX_LENGTH];
static uint32_t gConsoleCommandLength = 0U;
static bool gConsoleDiscardLine = false;
static char gConsolePreviousChar = '\0';
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

static int8_t consoleRunCommand(const stConsoleCommand *entry, char *command)
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
            if (consoleRunCommand(gConsoleRegisteredCommands[lIndex], lCommand) != CONSOLE_STATUS_OK) {
                LOG_W(gConsoleTag, "command failed: %s", lCommand);
            }
            return;
        }
    }

    LOG_W(gConsoleTag, "unknown command: %s", lCommand);
}

int8_t consoleRegisterCommand(const stConsoleCommand *command)
{
    uint32_t lIndex;

    if ((command == NULL) || (command->name == NULL) || (command->name[0] == '\0') ||
        (command->description == NULL) || (command->handler == NULL)) {
        return CONSOLE_STATUS_INVALID_ARGUMENT;
    }

    for (lIndex = 0U; lIndex < gConsoleRegisteredCommandCount; lIndex++) {
        if (gConsoleRegisteredCommands[lIndex] == command) {
            return CONSOLE_STATUS_OK;
        }
        if (consoleCommandNameEqual(command->name, gConsoleRegisteredCommands[lIndex]->name)) {
            return CONSOLE_STATUS_DUPLICATE;
        }
    }

    if (gConsoleRegisteredCommandCount >= CONSOLE_REGISTERED_COMMAND_MAX) {
        return CONSOLE_STATUS_FULL;
    }

    gConsoleRegisteredCommands[gConsoleRegisteredCommandCount++] = command;
    return CONSOLE_STATUS_OK;
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

/** Echo typed input back to the console link only; the default build leaves this to the host. */
static void consoleEcho(const stLogPortOps *ops, const char *text, uint32_t length)
{
#if CONSOLE_ECHO_ENABLE
    if (ops->write != NULL) {
        (void)ops->write((const uint8_t *)text, length);
    }
#else
    (void)ops;
    (void)text;
    (void)length;
#endif
}

static void consolePushChar(const stLogPortOps *ops, char value)
{
    char lPrevious = gConsolePreviousChar;

    gConsolePreviousChar = value;
    if ((value == '\n') && (lPrevious == '\r')) {
        return;
    }

    if ((value == '\r') || (value == '\n')) {
        consoleEcho(ops, "\r\n", 2U);
        if (!gConsoleDiscardLine) {
            gConsoleCommandBuffer[gConsoleCommandLength] = '\0';
            consoleDispatchCommand(gConsoleCommandBuffer);
        }
        gConsoleCommandLength = 0U;
        gConsoleDiscardLine = false;
        return;
    }

    /* After an overflow the rest of the line is dropped, so its tail never runs as a command. */
    if (gConsoleDiscardLine) {
        return;
    }

    if ((value == '\b') || (value == (char)0x7F)) {
        if (gConsoleCommandLength > 0U) {
            gConsoleCommandLength--;
            consoleEcho(ops, "\b \b", 3U);
        }
        return;
    }

    if (gConsoleCommandLength >= (CONSOLE_COMMAND_MAX_LENGTH - 1U)) {
        gConsoleDiscardLine = true;
        LOG_W(gConsoleTag, "command too long");
        return;
    }

    gConsoleCommandBuffer[gConsoleCommandLength++] = value;
    consoleEcho(ops, &value, 1U);
}

void consoleProcess(void)
{
    const stLogPortOps *lOps = logPortGetOps(CONSOLE_INPUT_LINK);
    uint8_t lReadBuffer[CONSOLE_READ_CHUNK_SIZE];
    uint32_t lReadLength;
    uint32_t lIndex;

    if ((lOps == NULL) || (lOps->read == NULL)) {
        return;
    }

    do {
        lReadLength = lOps->read(lReadBuffer, (uint32_t)sizeof(lReadBuffer));
        if (lReadLength > (uint32_t)sizeof(lReadBuffer)) {
            lReadLength = (uint32_t)sizeof(lReadBuffer);
        }
        for (lIndex = 0U; lIndex < lReadLength; lIndex++) {
            consolePushChar(lOps, (char)lReadBuffer[lIndex]);
        }
    } while (lReadLength == (uint32_t)sizeof(lReadBuffer));
}

int8_t consoleDrain(uint32_t maxPending, uint32_t timeoutMs)
{
    const stLogPortOps *lOps = logPortGetOps(CONSOLE_INPUT_LINK);
    const uint32_t lStartMs = logPortGetTimeMs();

    if (lOps == NULL) {
        return CONSOLE_STATUS_NO_LINK;
    }
    if (lOps->pending == NULL) {
        return CONSOLE_STATUS_OK;
    }

    /* Only the console link is awaited: the host sits there, other links are best effort. */
    while (lOps->pending() > maxPending) {
        if ((logPortGetTimeMs() - lStartMs) >= timeoutMs) {
            return CONSOLE_STATUS_TIMEOUT;
        }
    }

    return CONSOLE_STATUS_OK;
}

/**************************End of file********************************/
