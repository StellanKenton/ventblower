/************************************************************************************
* @file     : portlog.h
* @brief    : Logging port configuration.
* @details  : Selects the compiled log level, line sizes, output links and the console link.
* @author   : Stellan
* @date     :
* @version  :
* @copyright: Copyright (c) 2026 ResMed. All rights reserved.
***********************************************************************************/
#ifndef PORT_LOG_H
#define PORT_LOG_H

/* Compile-time level filter: 0 none, 1 error, 2 warn, 3 info, 4 debug. */
#ifndef LOG_COMPILED_LEVEL
#define LOG_COMPILED_LEVEL      4
#endif

/* Line buffers live on the caller stack; LOG_R also carries long test data lines. */
#ifndef LOG_LINE_MAX_SIZE
#define LOG_LINE_MAX_SIZE       256U
#endif
#ifndef LOG_RAW_LINE_MAX_SIZE
#define LOG_RAW_LINE_MAX_SIZE   384U
#endif

/* Link ids are fixed; a disabled link keeps its id and is skipped. */
#define LOG_LINK_RTT            0U
#define LOG_LINK_UART           1U
#define LOG_LINK_COUNT          2U

#ifndef LOG_LINK_RTT_ENABLE
#define LOG_LINK_RTT_ENABLE     1
#endif
/* Needs a dedicated USART with non-blocking stream TX; see log.md before enabling. */
#ifndef LOG_LINK_UART_ENABLE
#define LOG_LINK_UART_ENABLE    0
#endif

#ifndef LOG_CONSOLE_ENABLE
#define LOG_CONSOLE_ENABLE      1
#endif
/* The console reads commands from exactly one link. */
#ifndef CONSOLE_INPUT_LINK
#define CONSOLE_INPUT_LINK      LOG_LINK_RTT
#endif
/* Set to 1 when a human types on a serial terminal without local echo. */
#ifndef CONSOLE_ECHO_ENABLE
#define CONSOLE_ECHO_ENABLE     0
#endif

#if LOG_CONSOLE_ENABLE && \
    ((CONSOLE_INPUT_LINK >= LOG_LINK_COUNT) || \
     ((CONSOLE_INPUT_LINK == LOG_LINK_RTT) && !LOG_LINK_RTT_ENABLE) || \
     ((CONSOLE_INPUT_LINK == LOG_LINK_UART) && !LOG_LINK_UART_ENABLE))
#error "CONSOLE_INPUT_LINK must name an enabled link"
#endif

#endif
/**************************End of file********************************/
