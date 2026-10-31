/************************************************************************************
* @file     : m24512r.c
* @brief    : M24512-R EEPROM driver.
* @details  : Uses software I2C on PD10/PD11 with a 7-bit device address of 0x50.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "m24512r.h"

#include <stddef.h>

#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"
#include "system_gd32f4xx.h"

static uint8_t gM24512rReady = 0U;

static void m24512rDelayUs(uint32_t delayUs) {
    uint32_t lStart = DWT->CYCCNT;
    uint32_t lCycles = (SystemCoreClock / 1000000U) * delayUs;

    while ((uint32_t)(DWT->CYCCNT - lStart) < lCycles) {
    }
}

static void m24512rDelay(void) {
    m24512rDelayUs(M24512R_HALF_PERIOD_US);
}

static void m24512rSetScl(uint8_t high) {
    gpio_bit_write(GPIOD, M24512R_SCL_PIN, (high != 0U) ? SET : RESET);
}

static void m24512rSetSda(uint8_t high) {
    gpio_bit_write(GPIOD, M24512R_SDA_PIN, (high != 0U) ? SET : RESET);
}

static void m24512rStart(void) {
    m24512rSetSda(1U);
    m24512rSetScl(1U);
    m24512rDelay();
    m24512rSetSda(0U);
    m24512rDelay();
    m24512rSetScl(0U);
    m24512rDelay();
}

static void m24512rStop(void) {
    m24512rSetSda(0U);
    m24512rDelay();
    m24512rSetScl(1U);
    m24512rDelay();
    m24512rSetSda(1U);
    m24512rDelay();
}

static int8_t m24512rWriteByte(uint8_t value) {
    uint8_t lMask;

    for (lMask = 0x80U; lMask != 0U; lMask >>= 1U) {
        m24512rSetSda(((value & lMask) != 0U) ? 1U : 0U);
        m24512rDelay();
        m24512rSetScl(1U);
        m24512rDelay();
        m24512rSetScl(0U);
        m24512rDelay();
    }

    m24512rSetSda(1U);
    m24512rDelay();
    m24512rSetScl(1U);
    m24512rDelay();
    if (gpio_input_bit_get(GPIOD, M24512R_SDA_PIN) != RESET) {
        m24512rSetScl(0U);
        m24512rDelay();
        return M24512R_ERROR_NACK;
    }
    m24512rSetScl(0U);
    m24512rDelay();
    return M24512R_STATUS_OK;
}

static uint8_t m24512rReadByte(uint8_t acknowledge) {
    uint8_t lIndex;
    uint8_t lValue = 0U;

    m24512rSetSda(1U);
    for (lIndex = 0U; lIndex < 8U; lIndex++) {
        lValue <<= 1U;
        m24512rSetScl(1U);
        m24512rDelay();
        if (gpio_input_bit_get(GPIOD, M24512R_SDA_PIN) != RESET) {
            lValue |= 1U;
        }
        m24512rSetScl(0U);
        m24512rDelay();
    }

    m24512rSetSda((acknowledge != 0U) ? 0U : 1U);
    m24512rDelay();
    m24512rSetScl(1U);
    m24512rDelay();
    m24512rSetScl(0U);
    m24512rDelay();
    m24512rSetSda(1U);
    return lValue;
}

static int8_t m24512rSendAddress(uint8_t read) {
    return m24512rWriteByte((uint8_t)((M24512R_DEVICE_ADDRESS_7BIT << 1U) | read));
}

static int8_t m24512rSendMemoryAddress(uint16_t address) {
    int8_t lStatus = m24512rWriteByte((uint8_t)(address >> 8U));

    if (lStatus == M24512R_STATUS_OK) {
        lStatus = m24512rWriteByte((uint8_t)address);
    }
    return lStatus;
}

static int8_t m24512rWaitWriteComplete(void) {
    uint32_t lStart = DWT->CYCCNT;
    uint32_t lTimeoutCycles = (SystemCoreClock / 1000000U) * M24512R_WRITE_TIMEOUT_US;
    int8_t lStatus;

    do {
        m24512rStart();
        lStatus = m24512rSendAddress(0U);
        m24512rStop();
        if (lStatus == M24512R_STATUS_OK) {
            return M24512R_STATUS_OK;
        }
    } while ((uint32_t)(DWT->CYCCNT - lStart) < lTimeoutCycles);

    return M24512R_ERROR_TIMEOUT;
}

static int8_t m24512rWritePage(uint16_t address, const uint8_t *data, uint16_t length) {
    uint16_t lIndex;
    int8_t lStatus;

    m24512rStart();
    lStatus = m24512rSendAddress(0U);
    if (lStatus == M24512R_STATUS_OK) {
        lStatus = m24512rSendMemoryAddress(address);
    }
    for (lIndex = 0U; (lStatus == M24512R_STATUS_OK) && (lIndex < length); lIndex++) {
        lStatus = m24512rWriteByte(data[lIndex]);
    }
    m24512rStop();

    return (lStatus == M24512R_STATUS_OK) ? m24512rWaitWriteComplete() : lStatus;
}

int8_t m24512rInit(void) {
    uint8_t lIndex;

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    rcu_periph_clock_enable(RCU_GPIOD);
    gpio_bit_set(GPIOD, M24512R_SCL_PIN | M24512R_SDA_PIN);
    gpio_mode_set(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, M24512R_SCL_PIN | M24512R_SDA_PIN);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, M24512R_SCL_PIN | M24512R_SDA_PIN);

    /* Release a slave that was interrupted in the middle of a transfer. */
    for (lIndex = 0U; lIndex < 9U; lIndex++) {
        m24512rSetScl(0U);
        m24512rDelay();
        m24512rSetScl(1U);
        m24512rDelay();
    }
    m24512rStop();
    gM24512rReady = 1U;
    return M24512R_STATUS_OK;
}

int8_t m24512rReadBytes(uint16_t address, uint8_t *data, uint16_t length) {
    uint16_t lIndex;
    int8_t lStatus;

    if ((data == NULL) || (length == 0U) || (((uint32_t)address + length) > M24512R_CAPACITY_BYTES)) {
        return M24512R_ERROR_INVALID_PARAM;
    }
    if (gM24512rReady == 0U) {
        return M24512R_ERROR_NOT_READY;
    }

    m24512rStart();
    lStatus = m24512rSendAddress(0U);
    if (lStatus == M24512R_STATUS_OK) {
        lStatus = m24512rSendMemoryAddress(address);
    }
    if (lStatus == M24512R_STATUS_OK) {
        m24512rStart();
        lStatus = m24512rSendAddress(1U);
    }
    for (lIndex = 0U; (lStatus == M24512R_STATUS_OK) && (lIndex < length); lIndex++) {
        data[lIndex] = m24512rReadByte((lIndex + 1U < length) ? 1U : 0U);
    }
    m24512rStop();
    return lStatus;
}

int8_t m24512rWriteBytes(uint16_t address, const uint8_t *data, uint16_t length) {
    uint16_t lChunk;
    uint16_t lPageRemaining;
    int8_t lStatus;

    if ((data == NULL) || (length == 0U) || (((uint32_t)address + length) > M24512R_CAPACITY_BYTES)) {
        return M24512R_ERROR_INVALID_PARAM;
    }
    if (gM24512rReady == 0U) {
        return M24512R_ERROR_NOT_READY;
    }

    while (length > 0U) {
        lPageRemaining = (uint16_t)(M24512R_PAGE_SIZE - (address % M24512R_PAGE_SIZE));
        lChunk = (length < lPageRemaining) ? length : lPageRemaining;
        lStatus = m24512rWritePage(address, data, lChunk);
        if (lStatus != M24512R_STATUS_OK) {
            return lStatus;
        }
        address = (uint16_t)(address + lChunk);
        data += lChunk;
        length = (uint16_t)(length - lChunk);
    }
    return M24512R_STATUS_OK;
}

/**************************End of file********************************/
