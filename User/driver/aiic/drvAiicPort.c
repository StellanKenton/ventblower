/************************************************************************************
* @file     : drvAiicPort.c
* @brief    : Project binding for software I2C.
* @details  : Maps AIIC bus lines to BSP GPIO pins.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "drvAiicPort.h"

#include <stddef.h>

#include "bsp_gpio.h"
#include "delay.h"

static uint8_t gDrvAiicPortBspReady = 0U;

static uint8_t drvAiicPortGetScl(uint8_t busId) {
    if (busId == DRV_AIIC_BUS_TCA9535) {
        return (uint8_t)BSP_GPIO_MCU_TCA9535_SCL;
    }
    if (busId == DRV_AIIC_BUS_TOUCH) {
        return (uint8_t)BSP_GPIO_MCU_TOUCH_SCL;
    }
    return (uint8_t)BSP_GPIO_MAX;
}

static uint8_t drvAiicPortGetSda(uint8_t busId) {
    if (busId == DRV_AIIC_BUS_TCA9535) {
        return (uint8_t)BSP_GPIO_MCU_TCA9535_SDA;
    }
    if (busId == DRV_AIIC_BUS_TOUCH) {
        return (uint8_t)BSP_GPIO_MCU_TOUCH_SDA;
    }
    return (uint8_t)BSP_GPIO_MAX;
}

static uint8_t drvAiicPortIsValidBus(uint8_t busId) {
    return ((busId == DRV_AIIC_BUS_TCA9535) || (busId == DRV_AIIC_BUS_TOUCH)) ? 1U : 0U;
}

static void drvAiicPortInitTouch(void) {
    (void)bspGpioSetMode(BSP_GPIO_MCU_TOUCH_INT, BSP_GPIO_DIR_OUTPUT);
    (void)bspGpioWrite(BSP_GPIO_MCU_TOUCH_INT, 1U);
    (void)bspGpioSetMode(BSP_GPIO_MCU_TOUCH_RST, BSP_GPIO_DIR_OUTPUT);
    (void)bspGpioWrite(BSP_GPIO_MCU_TOUCH_RST, 0U);
    delay_ms(10U);
    (void)bspGpioWrite(BSP_GPIO_MCU_TOUCH_RST, 1U);
    delay_ms(60U);
    (void)bspGpioSetMode(BSP_GPIO_MCU_TOUCH_INT, BSP_GPIO_DIR_INPUT);
}

static int8_t drvAiicPortInitImpl(uint8_t busId) {
    uint8_t lScl;
    uint8_t lSda;

    if (drvAiicPortIsValidBus(busId) == 0U) {
        return DRV_AIIC_STATUS_INVALID_PARAM;
    }

    if (gDrvAiicPortBspReady == 0U) {
        bspGpioInit();
        gDrvAiicPortBspReady = 1U;
    }

    if (busId == DRV_AIIC_BUS_TOUCH) {
        drvAiicPortInitTouch();
    }

    lScl = drvAiicPortGetScl(busId);
    lSda = drvAiicPortGetSda(busId);
    (void)bspGpioSetMode((BspGpioId)lScl, BSP_GPIO_DIR_OUTPUT);
    (void)bspGpioSetMode((BspGpioId)lSda, BSP_GPIO_DIR_INPUT);
    (void)bspGpioWrite((BspGpioId)lScl, 1U);
    return DRV_AIIC_STATUS_OK;
}

static void drvAiicPortSetSclImpl(uint8_t busId, uint8_t level) {
    uint8_t lScl;

    if (drvAiicPortIsValidBus(busId) == 0U) {
        return;
    }

    lScl = drvAiicPortGetScl(busId);
    (void)bspGpioSetMode((BspGpioId)lScl, BSP_GPIO_DIR_OUTPUT);
    (void)bspGpioWrite((BspGpioId)lScl, (level != 0U) ? 1U : 0U);
}

static void drvAiicPortSetSdaImpl(uint8_t busId, uint8_t level) {
    uint8_t lSda;

    if (drvAiicPortIsValidBus(busId) == 0U) {
        return;
    }

    lSda = drvAiicPortGetSda(busId);
    if (level != 0U) {
        (void)bspGpioSetMode((BspGpioId)lSda, BSP_GPIO_DIR_INPUT);
    } else {
        (void)bspGpioSetMode((BspGpioId)lSda, BSP_GPIO_DIR_OUTPUT);
        (void)bspGpioWrite((BspGpioId)lSda, 0U);
    }
}

static uint8_t drvAiicPortReadSdaImpl(uint8_t busId) {
    uint8_t lSda;

    if (drvAiicPortIsValidBus(busId) == 0U) {
        return 1U;
    }

    lSda = drvAiicPortGetSda(busId);
    (void)bspGpioSetMode((BspGpioId)lSda, BSP_GPIO_DIR_INPUT);
    return bspGpioRead((BspGpioId)lSda);
}

static void drvAiicPortDelayUsImpl(uint16_t delayUs) {
    delay_us(delayUs);
}

static const stDrvAiicPortOps gDrvAiicPortOps = {
    .init = drvAiicPortInitImpl,
    .setScl = drvAiicPortSetSclImpl,
    .setSda = drvAiicPortSetSdaImpl,
    .readSda = drvAiicPortReadSdaImpl,
    .delayUs = drvAiicPortDelayUsImpl,
};

const stDrvAiicPortOps *drvAiicPortGetOps(uint8_t busId) {
    if (drvAiicPortIsValidBus(busId) == 0U) {
        return NULL;
    }

    return &gDrvAiicPortOps;
}

/**************************End of file********************************/
