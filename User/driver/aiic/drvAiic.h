/************************************************************************************
* @file     : drvAiic.h
* @brief    : Software I2C master driver.
* @details  :
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef DRV_AIIC_H
#define DRV_AIIC_H

#include <stdbool.h>
#include <stdint.h>

#include "rtos.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Status reason codes. */
#define DRV_AIIC_STATUS_OK                     1
#define DRV_AIIC_STATUS_INVALID_PARAM        (-1)
#define DRV_AIIC_STATUS_NOT_READY            (-2)
#define DRV_AIIC_STATUS_BUSY                 (-3)
#define DRV_AIIC_STATUS_NACK                 (-4)
#define DRV_AIIC_STATUS_BUS_ERROR            (-5)
#define DRV_AIIC_STATUS_ERROR                (-6)

typedef void (*pfDrvAiicSetLine)(uint8_t busId, uint8_t level);
typedef uint8_t (*pfDrvAiicReadLine)(uint8_t busId);
typedef void (*pfDrvAiicDelayUs)(uint16_t delayUs);
typedef int8_t (*pfDrvAiicPortInit)(uint8_t busId);

typedef struct stDrvAiicPortOps {
    pfDrvAiicPortInit init;
    pfDrvAiicSetLine setScl;
    pfDrvAiicSetLine setSda;
    pfDrvAiicReadLine readSda;
    pfDrvAiicDelayUs delayUs;
} stDrvAiicPortOps;

typedef struct stDrvAiicDeviceConfig {
    uint8_t busId;
    uint8_t devAddr7;
    uint16_t halfPeriodUs;
    const stDrvAiicPortOps *ops;
} stDrvAiicDeviceConfig;

typedef struct stDrvAiicDevice {
    uint8_t isReady;
    uint8_t busId;
    uint8_t devAddr7;
    uint16_t halfPeriodUs;
    const stDrvAiicPortOps *ops;
} stDrvAiicDevice;

typedef struct stDrvAiicBusState {
    uint8_t isReady;
    stRepRtosMutex lock;
} stDrvAiicBusState;

int8_t drvAiicInit(stDrvAiicDevice *device, const stDrvAiicDeviceConfig *config);
int8_t drvAiicDeinit(stDrvAiicDevice *device);
int8_t drvAiicWrite(stDrvAiicDevice *device, const uint8_t *data, uint16_t length);
int8_t drvAiicRead(stDrvAiicDevice *device, uint8_t *data, uint16_t length);
int8_t drvAiicWriteRead(stDrvAiicDevice *device, const uint8_t *txData, uint16_t txLength, uint8_t *rxData, uint16_t rxLength);
int8_t drvAiicRecoverBus(stDrvAiicDevice *device);

#ifdef __cplusplus
}
#endif

#endif
/**************************End of file********************************/
