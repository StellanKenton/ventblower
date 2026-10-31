/************************************************************************************
* @file     : console.h
* @brief    : RTT console command interface.
* @details  : Provides command binding types and the console processing entry.
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

#define CONSOLE_COMMAND_MAX_LENGTH 256U
#define CONSOLE_RTT_READ_BUFFER_SIZE 256U
#define CONSOLE_REGISTERED_COMMAND_MAX 32U

typedef enum eConsoleCommandResult {
    CONSOLE_COMMAND_RESULT_OK = 0,
    CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT,
    CONSOLE_COMMAND_RESULT_ERROR,
} eConsoleCommandResult;

typedef eConsoleCommandResult (*pfConsoleCommandHandler)(const char *arguments);

/* Command descriptors must remain valid for the entire console lifetime. */
typedef struct stConsoleCommand {
    const char *name;
    const char *description;
    pfConsoleCommandHandler handler;
} stConsoleCommand;

bool consoleProcess(uint16_t TickMs);
bool consoleRegisterCommand(const stConsoleCommand *command);
void consoleShowHelp(void);

#ifdef __cplusplus
}
#endif

#endif
/**************************End of file********************************/
