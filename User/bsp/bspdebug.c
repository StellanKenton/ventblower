/************************************************************************************
* @file     : bspdebug.c
* @brief    : BSP debug console command implementation.
* @details  : Reads raw ADC samples and gets or sets digital valve states.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "bspdebug.h"

#include <stddef.h>
#include <stdint.h>

#include "adc.h"
#include "blower_vcm.h"
#include "console.h"
#include "dvalve.h"
#include "log.h"
#include "rtos.h"
#include "valve.h"

static const char *const gBspDebugTag = "bsp";

static const char *const gBspDebugValveNames[VALVE_COUNT] = {
    "insp",
    "exp",
    "diff",
    "flush"
};

static const char *const gBspDebugDvalveNames[DVALVE_COUNT] = {
    "o2",
    "relief",
    "exp"
};

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
 * @brief Find a name in a fixed string table.
 * @param token Token start address.
 * @param length Token length.
 * @param names String table.
 * @param count String table entry count.
 * @return Matching index, or -1 when not found.
 */
static int32_t bspDebugNameFind(const char *token, uint32_t length, const char *const *names, uint32_t count)
{
    uint32_t lIndex;

    for (lIndex = 0U; lIndex < count; lIndex++) {
        if (bspDebugTokenEqual(token, length, names[lIndex])) {
            return (int32_t)lIndex;
        }
    }

    return -1;
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
    LOG_I(gBspDebugTag, "valve: bsp v|valve <insp|exp|diff|flush> [0|1]");
    LOG_I(gBspDebugTag, "dvalve: bsp d|dv|dvalve <o2|relief|exp> <0-100>");
    LOG_I(gBspDebugTag, "blower: bsp blower <speed 0-800|pwm 0-100|stats>");
}

/**
 * @brief Print the latest raw phase current, VBUS and NTC samples.
 * @param arguments Arguments following the ADC subcommand.
 * @return Console command result.
 */
static eConsoleCommandResult bspDebugAdcCommand(const char *arguments)
{
    const char *lToken;
    uint32_t lLength;

    if (bspDebugTokenRead(&arguments, &lToken, &lLength)) {
        bspDebugUsageShow();
        return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
    }

    LOG_I(gBspDebugTag, "adc iu %u iv %u iw %u vbus %u ntc %u",
          (unsigned int)gAdcRaw[ADC_CH_IU],
          (unsigned int)gAdcRaw[ADC_CH_IV],
          (unsigned int)gAdcRaw[ADC_CH_IW],
          (unsigned int)gAdcRaw[ADC_CH_VBUS],
          (unsigned int)gAdcRaw[ADC_CH_NTC]);
    return CONSOLE_COMMAND_RESULT_OK;
}

/**
 * @brief Handle a digital valve state query or control request.
 * @param arguments Arguments following the valve subcommand.
 * @return Console command result.
 */
static eConsoleCommandResult bspDebugValveCommand(const char *arguments)
{
    const char *lToken;
    uint32_t lLength;
    int32_t lIndex;
    eValveLevel lLevel;
    int8_t lStatus;

    if (!bspDebugTokenRead(&arguments, &lToken, &lLength)) {
        bspDebugUsageShow();
        return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
    }
    lIndex = bspDebugNameFind(lToken, lLength, gBspDebugValveNames, (uint32_t)VALVE_COUNT);
    if (lIndex < 0) {
        bspDebugUsageShow();
        return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
    }

    if (!bspDebugTokenRead(&arguments, &lToken, &lLength)) {
        lLevel = valveStateGet((eValveIndex)lIndex);
        if (lLevel == VALVE_LEVEL_INVALID) {
            LOG_E(gBspDebugTag, "valve %s read failed", gBspDebugValveNames[lIndex]);
            return CONSOLE_COMMAND_RESULT_ERROR;
        }
        LOG_I(gBspDebugTag, "valve %s %u", gBspDebugValveNames[lIndex], (unsigned int)lLevel);
        return CONSOLE_COMMAND_RESULT_OK;
    }

    if ((lLength != 1U) || ((lToken[0] != '0') && (lToken[0] != '1')) ||
        bspDebugTokenRead(&arguments, &lToken, &lLength)) {
        bspDebugUsageShow();
        return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
    }

    lLevel = (lToken[0] == '1') ? VALVE_LEVEL_HIGH : VALVE_LEVEL_LOW;
    lStatus = valveControlSet((eValveIndex)lIndex, lLevel);
    if (lStatus != VALVE_STATUS_OK) {
        LOG_E(gBspDebugTag, "valve %s set failed: %d", gBspDebugValveNames[lIndex], (int)lStatus);
        return CONSOLE_COMMAND_RESULT_ERROR;
    }
    LOG_I(gBspDebugTag, "valve %s set %u", gBspDebugValveNames[lIndex], (unsigned int)lLevel);
    return CONSOLE_COMMAND_RESULT_OK;
}

/**
 * @brief Handle a proportional valve PWM duty request.
 * @param arguments Arguments following the dvalve subcommand.
 * @return Console command result.
 */
static eConsoleCommandResult bspDebugDvalveCommand(const char *arguments)
{
    const char *lToken;
    uint32_t lLength;
    int32_t lIndex;
    uint16_t lDuty;
    int8_t lStatus;

    if (!bspDebugTokenRead(&arguments, &lToken, &lLength)) {
        bspDebugUsageShow();
        return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
    }
    lIndex = bspDebugNameFind(lToken, lLength, gBspDebugDvalveNames, (uint32_t)DVALVE_COUNT);
    if ((lIndex < 0) || !bspDebugTokenRead(&arguments, &lToken, &lLength)) {
        bspDebugUsageShow();
        return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
    }
    if (!bspDebugUint16Parse(lToken, lLength, DVALVE_DUTY_MAX_PERCENT, &lDuty) ||
        bspDebugTokenRead(&arguments, &lToken, &lLength)) {
        bspDebugUsageShow();
        return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
    }

    lStatus = dvalveDutySet((eDvalveIndex)lIndex, (uint8_t)lDuty);
    if (lStatus != DVALVE_STATUS_OK) {
        LOG_E(gBspDebugTag, "dvalve %s set failed: %d", gBspDebugDvalveNames[lIndex], (int)lStatus);
        return CONSOLE_COMMAND_RESULT_ERROR;
    }
    LOG_I(gBspDebugTag, "dvalve %s %u%%", gBspDebugDvalveNames[lIndex], (unsigned int)lDuty);
    return CONSOLE_COMMAND_RESULT_OK;
}

/**
 * @brief Handle a VCM blower speed or PWM control request.
 * @param arguments Arguments following the blower subcommand.
 * @return Console command result.
 */
static eConsoleCommandResult bspDebugBlowerCommand(const char *arguments)
{
    const char *lModeToken;
    const char *lValueToken;
    uint32_t lModeLength;
    uint32_t lValueLength;
    uint16_t lValue;
    uint16_t lTarget;
    eBlowerVcmControlMode lMode;
    stBlowerVcmStats lStats;
    stBlowerVcmFeedback lFeedback;

    if (!bspDebugTokenRead(&arguments, &lModeToken, &lModeLength)) {
        bspDebugUsageShow();
        return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
    }

    if (bspDebugTokenEqual(lModeToken, lModeLength, "stats") &&
        !bspDebugTokenRead(&arguments, &lValueToken, &lValueLength)) {
        (void)blowerVcmGetStats(&lStats);
        (void)blowerVcmGetFeedback(&lFeedback);
        LOG_I(gBspDebugTag,
              "blower link connected=%u valid=%u speed_x10=%u rx_bytes=%lu rx_frames=%lu tx_frames=%lu",
              blowerVcmIsConnected(repRtosGetTickMs()) ? 1U : 0U,
              lFeedback.valid ? 1U : 0U,
              (unsigned int)lFeedback.speedScaled,
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
        return CONSOLE_COMMAND_RESULT_OK;
    }
    if (!bspDebugTokenRead(&arguments, &lValueToken, &lValueLength) ||
        bspDebugTokenRead(&arguments, &lValueToken, &lValueLength)) {
        bspDebugUsageShow();
        return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
    }

    if (bspDebugTokenEqual(lModeToken, lModeLength, "speed") &&
        bspDebugUint16Parse(lValueToken, lValueLength, BSP_DEBUG_BLOWER_SPEED_MAX_RPS, &lValue)) {
        lMode = BLOWER_CTRL_SPEED;
        lTarget = lValue;
    } else if (bspDebugTokenEqual(lModeToken, lModeLength, "pwm") &&
               bspDebugUint16Parse(lValueToken, lValueLength, BSP_DEBUG_BLOWER_PWM_MAX_PERCENT, &lValue)) {
        lMode = BLOWER_CTRL_PWM;
        lTarget = (uint16_t)(lValue * BSP_DEBUG_BLOWER_PWM_SCALE);
    } else {
        bspDebugUsageShow();
        return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
    }

    /* SensorTask keeps the last control frame alive through blowerVcmProcess(). */
    if (blowerVcmSendControl(lMode, (uint16_t)(lTarget * 10U), 0U) != BLOWER_VCM_STATUS_OK) {
        LOG_E(gBspDebugTag, "blower control send failed");
        return CONSOLE_COMMAND_RESULT_ERROR;
    }
    LOG_I(gBspDebugTag, "blower %s %u", (lMode == BLOWER_CTRL_SPEED) ? "speed" : "pwm",
          (unsigned int)lValue);
    return CONSOLE_COMMAND_RESULT_OK;
}

/**
 * @brief Dispatch the BSP debug subcommand.
 * @param arguments Arguments following the bsp command.
 * @return Console command result.
 */
static eConsoleCommandResult bspDebugConsoleCommand(const char *arguments)
{
    const char *lToken;
    uint32_t lLength;

    if (!bspDebugTokenRead(&arguments, &lToken, &lLength)) {
        bspDebugUsageShow();
        return CONSOLE_COMMAND_RESULT_OK;
    }
    if (bspDebugTokenEqual(lToken, lLength, "a") || bspDebugTokenEqual(lToken, lLength, "adc")) {
        return bspDebugAdcCommand(arguments);
    }
    if (bspDebugTokenEqual(lToken, lLength, "v") || bspDebugTokenEqual(lToken, lLength, "valve")) {
        return bspDebugValveCommand(arguments);
    }
    if (bspDebugTokenEqual(lToken, lLength, "d") || bspDebugTokenEqual(lToken, lLength, "dv") ||
        bspDebugTokenEqual(lToken, lLength, "dvalve")) {
        return bspDebugDvalveCommand(arguments);
    }
    if (bspDebugTokenEqual(lToken, lLength, "blower")) {
        return bspDebugBlowerCommand(arguments);
    }

    bspDebugUsageShow();
    return CONSOLE_COMMAND_RESULT_INVALID_ARGUMENT;
}

static const stConsoleCommand gBspDebugConsoleCommand = {
    "bsp",
    "BSP debug: adc, valves, and blower control",
    bspDebugConsoleCommand
};

/**
 * @brief Register the BSP debug command with the RTT console.
 * @return true when registration succeeds.
 */
bool bspDebugConsoleRegister(void)
{
    return consoleRegisterCommand(&gBspDebugConsoleCommand);
}

/**************************End of file********************************/
