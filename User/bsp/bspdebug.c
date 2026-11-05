/************************************************************************************
* @file     : bspdebug.c
* @brief    : BSP debug console command implementation.
* @details  : Reads raw ADC samples and inspects the Core blower link.
* @author   : Stellan
* @date     :
* @version  :
* @copyright: Copyright (c) 2026 ResMed. All rights reserved.
***********************************************************************************/
#include "bspdebug.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "adc.h"
#include "blower_vcm.h"
#include "console.h"
#include "log.h"
#include "rtos.h"

static const char *const gBspDebugTag = "bsp";

/**
 * @brief Skip spaces in a command argument string.
 * @param arguments Current argument position.
 * @return First non-space position.
 */
static const char *bspDebugSkipSpaces(const char *arguments)
{
    while ((*arguments == ' ') || (*arguments == '\t')) {
        arguments++;
    }

    return arguments;
}

/**
 * @brief Read the next token without modifying the console buffer.
 * @param arguments Current argument position, updated after the token.
 * @param token Token start address.
 * @param length Token length.
 * @return true when a token was found.
 */
static bool bspDebugTokenRead(const char **arguments, const char **token, uint32_t *length)
{
    const char *lStart = bspDebugSkipSpaces(*arguments);
    const char *lEnd = lStart;

    if (*lStart == '\0') {
        return false;
    }
    while ((*lEnd != '\0') && (*lEnd != ' ') && (*lEnd != '\t')) {
        lEnd++;
    }

    *token = lStart;
    *length = (uint32_t)(lEnd - lStart);
    *arguments = lEnd;
    return true;
}

/**
 * @brief Compare a parsed token with a zero-terminated name.
 * @param token Token start address.
 * @param length Token length.
 * @param name Expected name.
 * @return true when both values match exactly.
 */
static bool bspDebugTokenEqual(const char *token, uint32_t length, const char *name)
{
    uint32_t lIndex;

    for (lIndex = 0U; lIndex < length; lIndex++) {
        if ((name[lIndex] == '\0') || (token[lIndex] != name[lIndex])) {
            return false;
        }
    }

    return name[length] == '\0';
}

/**
 * @brief Parse an unsigned decimal value with an inclusive upper bound.
 * @param token Token start address.
 * @param length Token length.
 * @param maximum Largest accepted value.
 * @param value Parsed value.
 * @return true when the complete token is a decimal value within the bound.
 */
static bool bspDebugUint16Parse(const char *token, uint32_t length, uint16_t maximum, uint16_t *value)
{
    uint32_t lIndex;
    uint32_t lValue = 0U;

    if (length == 0U) {
        return false;
    }
    for (lIndex = 0U; lIndex < length; lIndex++) {
        if ((token[lIndex] < '0') || (token[lIndex] > '9')) {
            return false;
        }
        lValue = (lValue * 10U) + (uint32_t)(token[lIndex] - '0');
        if (lValue > maximum) {
            return false;
        }
    }

    *value = (uint16_t)lValue;
    return true;
}

/**
 * @brief Print the compact BSP command syntax.
 */
static void bspDebugUsageShow(void)
{
    LOG_I(gBspDebugTag, "adc: bsp a|adc");
    LOG_I(gBspDebugTag, "blower: bsp blower <stats|speed 0-800>");
}

/**
 * @brief Print the latest raw phase current, VBUS and NTC samples.
 * @param arguments Arguments following the ADC subcommand.
 * @return Console command result.
 */
static int8_t bspDebugAdcCommand(const char *arguments)
{
    const char *lToken;
    uint32_t lLength;

    if (bspDebugTokenRead(&arguments, &lToken, &lLength)) {
        bspDebugUsageShow();
        return CONSOLE_STATUS_INVALID_ARGUMENT;
    }

    LOG_I(gBspDebugTag, "adc iu %u iv %u iw %u vbus %u ntc %u",
          (unsigned int)gAdcRaw[ADC_CH_IU],
          (unsigned int)gAdcRaw[ADC_CH_IV],
          (unsigned int)gAdcRaw[ADC_CH_IW],
          (unsigned int)gAdcRaw[ADC_CH_VBUS],
          (unsigned int)gAdcRaw[ADC_CH_NTC]);
    return CONSOLE_STATUS_OK;
}

/**
 * @brief Show the Core link state or override the reported speed feedback.
 * @param arguments Arguments following the blower subcommand.
 * @return Console command result.
 */
static int8_t bspDebugBlowerCommand(const char *arguments)
{
    const char *lModeToken;
    const char *lValueToken;
    uint32_t lModeLength;
    uint32_t lValueLength;
    uint16_t lValue;
    stBlowerVcmStats lStats;
    stBlowerVcmControl lControl;

    if (!bspDebugTokenRead(&arguments, &lModeToken, &lModeLength)) {
        bspDebugUsageShow();
        return CONSOLE_STATUS_INVALID_ARGUMENT;
    }

    if (bspDebugTokenEqual(lModeToken, lModeLength, "stats") &&
        !bspDebugTokenRead(&arguments, &lValueToken, &lValueLength)) {
        (void)blowerVcmGetStats(&lStats);
        (void)blowerVcmGetControl(&lControl);
        LOG_I(gBspDebugTag,
              "blower link connected=%u valid=%u mode=%u target=%u sat=%u rx_bytes=%lu rx_frames=%lu tx_frames=%lu",
              blowerVcmIsConnected(repRtosGetTickMs()) ? 1U : 0U,
              lControl.valid ? 1U : 0U,
              (unsigned int)lControl.mode,
              (unsigned int)lControl.targetValue,
              (unsigned int)lControl.vcmSaturation,
              (unsigned long)lStats.rxByteCount,
              (unsigned long)lStats.rxFrameCount,
              (unsigned long)lStats.txFrameCount);
        LOG_I(gBspDebugTag,
              "blower errors crc=%lu len=%lu tail=%lu data=%lu unknown=%lu overflow=%lu uart=%lu dma=%lu busy=%lu",
              (unsigned long)lStats.rxCrcErrorCount,
              (unsigned long)lStats.rxLengthErrorCount,
              (unsigned long)lStats.rxTailErrorCount,
              (unsigned long)lStats.rxDataErrorCount,
              (unsigned long)lStats.rxUnknownCommandCount,
              (unsigned long)lStats.rxOverflowCount,
              (unsigned long)lStats.uartErrorCount,
              (unsigned long)lStats.dmaErrorCount,
              (unsigned long)lStats.txBusyDropCount);
        return CONSOLE_STATUS_OK;
    }
    if (!bspDebugTokenEqual(lModeToken, lModeLength, "speed") ||
        !bspDebugTokenRead(&arguments, &lValueToken, &lValueLength) ||
        bspDebugTokenRead(&arguments, &lValueToken, &lValueLength) ||
        !bspDebugUint16Parse(lValueToken, lValueLength, BSP_DEBUG_BLOWER_SPEED_MAX_RPS, &lValue)) {
        bspDebugUsageShow();
        return CONSOLE_STATUS_INVALID_ARGUMENT;
    }

    /* Link bring-up only: the motor loop will own this value once it reports real speed. */
    (void)blowerVcmSetFeedback((uint16_t)(lValue * 10U), 0U);
    LOG_I(gBspDebugTag, "blower feedback speed %u", (unsigned int)lValue);
    return CONSOLE_STATUS_OK;
}

/**
 * @brief Dispatch the BSP debug subcommand.
 * @param arguments Arguments following the bsp command.
 * @return Console command result.
 */
static int8_t bspDebugConsoleCommand(const char *arguments)
{
    const char *lToken;
    uint32_t lLength;

    if (!bspDebugTokenRead(&arguments, &lToken, &lLength)) {
        bspDebugUsageShow();
        return CONSOLE_STATUS_OK;
    }
    if (bspDebugTokenEqual(lToken, lLength, "a") || bspDebugTokenEqual(lToken, lLength, "adc")) {
        return bspDebugAdcCommand(arguments);
    }
    if (bspDebugTokenEqual(lToken, lLength, "blower")) {
        return bspDebugBlowerCommand(arguments);
    }

    bspDebugUsageShow();
    return CONSOLE_STATUS_INVALID_ARGUMENT;
}

static const stConsoleCommand gBspDebugConsoleCommand = {
    "bsp",
    "BSP debug: adc and blower control",
    bspDebugConsoleCommand
};

/**
 * @brief Register the BSP debug command with the console.
 * @return CONSOLE_STATUS_OK, or the console error status.
 */
int8_t bspDebugConsoleRegister(void)
{
    return consoleRegisterCommand(&gBspDebugConsoleCommand);
}

/**************************End of file********************************/
