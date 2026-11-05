/************************************************************************************
* @file     : portlog.c
* @brief    : Logging port implementation.
* @details  : Binds the log links, the time stamp source and the ISR check for this project.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "log.h"

#include <stddef.h>

#include "gd32f4xx.h"
#include "rtos.h"
#include "SEGGER_RTT.h"

#if LOG_LINK_RTT_ENABLE
static void logPortRttInit(void)
{
    SEGGER_RTT_Init();
    /* SKIP keeps line boundaries: a line that does not fit is dropped as a whole. */
    (void)SEGGER_RTT_SetFlagsUpBuffer(0U, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
}

static uint32_t logPortRttWrite(const uint8_t *data, uint32_t length)
{
    return SEGGER_RTT_Write(0U, data, length);
}

static uint32_t logPortRttRead(uint8_t *data, uint32_t length)
{
    return SEGGER_RTT_Read(0U, data, length);
}

static uint32_t logPortRttPending(void)
{
    return SEGGER_RTT_GetBytesInBuffer(0U);
}

static const stLogPortOps gLogPortRttOps = {
    .name = "rtt",
    .init = logPortRttInit,
    .write = logPortRttWrite,
    .read = logPortRttRead,
    .pending = logPortRttPending,
};
#endif

/* A UART link needs gLogPortUartOps above, bound to a dedicated USART with stream TX (log.md). */
static const stLogPortOps *const gLogPortLinks[LOG_LINK_COUNT] = {
#if LOG_LINK_RTT_ENABLE
    [LOG_LINK_RTT] = &gLogPortRttOps,
#endif
#if LOG_LINK_UART_ENABLE
    [LOG_LINK_UART] = &gLogPortUartOps,
#endif
};

const stLogPortOps *logPortGetOps(uint8_t linkId)
{
    /* Disabled or unknown links return NULL and are skipped by the core. */
    return (linkId < LOG_LINK_COUNT) ? gLogPortLinks[linkId] : NULL;
}

uint32_t logPortGetTimeMs(void)
{
    return repRtosGetTickMs();
}

bool logPortIsInIsr(void)
{
    return __get_IPSR() != 0U;
}

/**************************End of file********************************/
