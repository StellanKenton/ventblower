/************************************************************************************
* @file     : console.h
* @brief    : Console command interface.
* @details  : Provides command binding types, line input processing and link draining.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CONSOLE_COMMAND_MAX_LENGTH
#define CONSOLE_COMMAND_MAX_LENGTH      256U
#endif
#ifndef CONSOLE_READ_CHUNK_SIZE
#define CONSOLE_READ_CHUNK_SIZE         32U
#endif
#ifndef CONSOLE_REGISTERED_COMMAND_MAX
#define CONSOLE_REGISTERED_COMMAND_MAX  32U
#endif

#define CONSOLE_STATUS_OK                   1
#define CONSOLE_STATUS_INVALID_ARGUMENT     (-1)
#define CONSOLE_STATUS_ERROR                (-2)
#define CONSOLE_STATUS_DUPLICATE            (-3)
#define CONSOLE_STATUS_FULL                 (-4)
#define CONSOLE_STATUS_NO_LINK              (-5)
#define CONSOLE_STATUS_TIMEOUT              (-6)

/* Handlers return CONSOLE_STATUS_OK or a negative status. */
typedef int8_t (*pfConsoleCommandHandler)(const char *arguments);

/* Command descriptors must remain valid for the entire console lifetime. */
typedef struct stConsoleCommand {
    const char *name;
    const char *description;
    pfConsoleCommandHandler handler;
} stConsoleCommand;

/** Read pending input from CONSOLE_INPUT_LINK and run complete lines. Call from one task. */
void consoleProcess(void);

/** Register a command; returns CONSOLE_STATUS_OK or a negative status. */
int8_t consoleRegisterCommand(const stConsoleCommand *command);

void consoleShowHelp(void);

/**
 * @brief Busy-wait until the console link holds at most maxPending unsent bytes.
 * @param maxPending Accepted backlog in bytes; 0 waits until everything is sent.
 * @param timeoutMs Upper bound for the wait.
 * @return CONSOLE_STATUS_OK, CONSOLE_STATUS_TIMEOUT or CONSOLE_STATUS_NO_LINK.
 * @note Task context only (needs a running tick); keep it in the low-priority console task.
 */
int8_t consoleDrain(uint32_t maxPending, uint32_t timeoutMs);

#ifdef __cplusplus
}
#endif

#endif
/**************************End of file********************************/
