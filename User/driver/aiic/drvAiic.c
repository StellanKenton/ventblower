/************************************************************************************
* @file     : drvAiic.c
* @brief    : Software I2C master driver.
* @details  : Implements start/stop, byte transfer, ACK handling and bus locking.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "drvAiic.h"

#include <stddef.h>

#include "drvAiicPort.h"

static stDrvAiicBusState gDrvAiicBusState[DRV_AIIC_MAX];

static uint8_t drvAiicIsValidDevice(const stDrvAiicDevice *device) {
    if ((device == NULL) || (device->isReady == 0U) || (device->ops == NULL)) {
        return 0U;
    }

    if ((device->ops->setScl == NULL) ||
        (device->ops->setSda == NULL) ||
        (device->ops->readSda == NULL) ||
        (device->ops->delayUs == NULL)) {
        return 0U;
    }

    return 1U;
}

static void drvAiicDelay(const stDrvAiicDevice *device) {
    device->ops->delayUs(device->halfPeriodUs);
}

static void drvAiicSetScl(const stDrvAiicDevice *device, uint8_t level) {
    device->ops->setScl(device->busId, level);
}

static void drvAiicSetSda(const stDrvAiicDevice *device, uint8_t level) {
    device->ops->setSda(device->busId, level);
}

static uint8_t drvAiicReadSda(const stDrvAiicDevice *device) {
    return device->ops->readSda(device->busId);
}

static int8_t drvAiicLock(stDrvAiicDevice *device) {
#if DRV_AIIC_USE_RTOS
    if (repRtosMutexTake(&gDrvAiicBusState[device->busId].lock, DRV_AIIC_LOCK_WAIT_MS) != REP_RTOS_STATUS_OK) {
        return DRV_AIIC_STATUS_BUSY;
    }
#else
    (void)device;
#endif

    return DRV_AIIC_STATUS_OK;
}

static void drvAiicUnlock(stDrvAiicDevice *device) {
#if DRV_AIIC_USE_RTOS
    (void)repRtosMutexGive(&gDrvAiicBusState[device->busId].lock);
#else
    (void)device;
#endif
}

static void drvAiicStart(const stDrvAiicDevice *device) {
    drvAiicSetSda(device, 1U);
    drvAiicSetScl(device, 1U);
    drvAiicDelay(device);
    drvAiicSetSda(device, 0U);
    drvAiicDelay(device);
    drvAiicSetScl(device, 0U);
    drvAiicDelay(device);
}

static void drvAiicStop(const stDrvAiicDevice *device) {
    drvAiicSetSda(device, 0U);
    drvAiicDelay(device);
    drvAiicSetScl(device, 1U);
    drvAiicDelay(device);
    drvAiicSetSda(device, 1U);
    drvAiicDelay(device);
}

static int8_t drvAiicWriteByte(const stDrvAiicDevice *device, uint8_t data) {
    uint8_t lMask;

    for (lMask = 0x80U; lMask != 0U; lMask >>= 1U) {
        drvAiicSetSda(device, ((data & lMask) != 0U) ? 1U : 0U);
        drvAiicDelay(device);
        drvAiicSetScl(device, 1U);
        drvAiicDelay(device);
        drvAiicSetScl(device, 0U);
        drvAiicDelay(device);
    }

    drvAiicSetSda(device, 1U);
    drvAiicDelay(device);
    drvAiicSetScl(device, 1U);
    drvAiicDelay(device);
    if (drvAiicReadSda(device) != 0U) {
        drvAiicSetScl(device, 0U);
        drvAiicDelay(device);
        return DRV_AIIC_STATUS_NACK;
    }

    drvAiicSetScl(device, 0U);
    drvAiicDelay(device);
    return DRV_AIIC_STATUS_OK;
}

static uint8_t drvAiicReadByte(const stDrvAiicDevice *device, uint8_t ack) {
    uint8_t lIndex;
    uint8_t lData;

    lData = 0U;
    drvAiicSetSda(device, 1U);
    for (lIndex = 0U; lIndex < 8U; lIndex++) {
        lData <<= 1U;
        drvAiicSetScl(device, 1U);
        drvAiicDelay(device);
        if (drvAiicReadSda(device) != 0U) {
            lData |= 1U;
        }
        drvAiicSetScl(device, 0U);
        drvAiicDelay(device);
    }

    drvAiicSetSda(device, (ack != 0U) ? 0U : 1U);
    drvAiicDelay(device);
    drvAiicSetScl(device, 1U);
    drvAiicDelay(device);
    drvAiicSetScl(device, 0U);
    drvAiicDelay(device);
    drvAiicSetSda(device, 1U);

    return lData;
}

static int8_t drvAiicSendAddress(const stDrvAiicDevice *device, uint8_t read) {
    return drvAiicWriteByte(device, (uint8_t)((device->devAddr7 << 1U) | ((read != 0U) ? 1U : 0U)));
}

int8_t drvAiicInit(stDrvAiicDevice *device, const stDrvAiicDeviceConfig *config) {
    const stDrvAiicPortOps *lOps;
    int8_t lStatus;

    if ((device == NULL) || (config == NULL) || (config->busId >= DRV_AIIC_MAX) || (config->devAddr7 > 0x7FU)) {
        return DRV_AIIC_STATUS_INVALID_PARAM;
    }

    lOps = (config->ops != NULL) ? config->ops : drvAiicPortGetOps(config->busId);
    if ((lOps == NULL) || (lOps->init == NULL)) {
        return DRV_AIIC_STATUS_NOT_READY;
    }

    lStatus = lOps->init(config->busId);
    if (lStatus != DRV_AIIC_STATUS_OK) {
        return lStatus;
    }

#if DRV_AIIC_USE_RTOS
    if ((gDrvAiicBusState[config->busId].isReady == 0U) &&
        (repRtosMutexCreate(&gDrvAiicBusState[config->busId].lock) != REP_RTOS_STATUS_OK)) {
        return DRV_AIIC_STATUS_ERROR;
    }
#endif

    gDrvAiicBusState[config->busId].isReady = 1U;
    device->isReady = 1U;
    device->busId = config->busId;
    device->devAddr7 = config->devAddr7;
    device->halfPeriodUs = (config->halfPeriodUs == 0U) ? DRV_AIIC_DEFAULT_HALF_PERIOD_US : config->halfPeriodUs;
    device->ops = lOps;

    return drvAiicRecoverBus(device);
}

int8_t drvAiicDeinit(stDrvAiicDevice *device) {
    if (device == NULL) {
        return DRV_AIIC_STATUS_INVALID_PARAM;
    }

    device->isReady = 0U;
    device->ops = NULL;
    return DRV_AIIC_STATUS_OK;
}

int8_t drvAiicWrite(stDrvAiicDevice *device, const uint8_t *data, uint16_t length) {
    uint16_t lIndex;
    int8_t lStatus;

    if ((drvAiicIsValidDevice(device) == 0U) || ((data == NULL) && (length > 0U))) {
        return DRV_AIIC_STATUS_INVALID_PARAM;
    }

    lStatus = drvAiicLock(device);
    if (lStatus != DRV_AIIC_STATUS_OK) {
        return lStatus;
    }

    drvAiicStart(device);
    lStatus = drvAiicSendAddress(device, 0U);
    for (lIndex = 0U; (lStatus == DRV_AIIC_STATUS_OK) && (lIndex < length); lIndex++) {
        lStatus = drvAiicWriteByte(device, data[lIndex]);
    }
    drvAiicStop(device);
    drvAiicUnlock(device);

    return lStatus;
}

int8_t drvAiicRead(stDrvAiicDevice *device, uint8_t *data, uint16_t length) {
    uint16_t lIndex;
    int8_t lStatus;

    if ((drvAiicIsValidDevice(device) == 0U) || (data == NULL) || (length == 0U)) {
        return DRV_AIIC_STATUS_INVALID_PARAM;
    }

    lStatus = drvAiicLock(device);
    if (lStatus != DRV_AIIC_STATUS_OK) {
        return lStatus;
    }

    drvAiicStart(device);
    lStatus = drvAiicSendAddress(device, 1U);
    for (lIndex = 0U; (lStatus == DRV_AIIC_STATUS_OK) && (lIndex < length); lIndex++) {
        data[lIndex] = drvAiicReadByte(device, (lIndex + 1U < length) ? 1U : 0U);
    }
    drvAiicStop(device);
    drvAiicUnlock(device);

    return lStatus;
}

int8_t drvAiicWriteRead(stDrvAiicDevice *device, const uint8_t *txData, uint16_t txLength, uint8_t *rxData, uint16_t rxLength) {
    uint16_t lIndex;
    int8_t lStatus;

    if ((drvAiicIsValidDevice(device) == 0U) ||
        ((txData == NULL) && (txLength > 0U)) ||
        (rxData == NULL) ||
        (rxLength == 0U)) {
        return DRV_AIIC_STATUS_INVALID_PARAM;
    }

    lStatus = drvAiicLock(device);
    if (lStatus != DRV_AIIC_STATUS_OK) {
        return lStatus;
    }

    drvAiicStart(device);
    lStatus = drvAiicSendAddress(device, 0U);
    for (lIndex = 0U; (lStatus == DRV_AIIC_STATUS_OK) && (lIndex < txLength); lIndex++) {
        lStatus = drvAiicWriteByte(device, txData[lIndex]);
    }

    if (lStatus == DRV_AIIC_STATUS_OK) {
        drvAiicStart(device);
        lStatus = drvAiicSendAddress(device, 1U);
    }

    for (lIndex = 0U; (lStatus == DRV_AIIC_STATUS_OK) && (lIndex < rxLength); lIndex++) {
        rxData[lIndex] = drvAiicReadByte(device, (lIndex + 1U < rxLength) ? 1U : 0U);
    }

    drvAiicStop(device);
    drvAiicUnlock(device);

    return lStatus;
}

int8_t drvAiicRecoverBus(stDrvAiicDevice *device) {
    uint8_t lIndex;

    if (drvAiicIsValidDevice(device) == 0U) {
        return DRV_AIIC_STATUS_INVALID_PARAM;
    }

    for (lIndex = 0U; lIndex < DRV_AIIC_DEFAULT_RECOVERY_CLOCKS; lIndex++) {
        drvAiicSetSda(device, 1U);
        drvAiicSetScl(device, 1U);
        drvAiicDelay(device);
        drvAiicSetScl(device, 0U);
        drvAiicDelay(device);
    }
    drvAiicStop(device);

    return DRV_AIIC_STATUS_OK;
}

/**************************End of file********************************/
