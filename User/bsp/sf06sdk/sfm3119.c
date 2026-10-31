/************************************************************************************
* @file     : sfm3119.c
* @brief    : Dual SFM3119 flow sensor interface.
* @details  : Manages the separate air and oxygen buses and cached results.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "sfm3119.h"

#include <stddef.h>

#include "gd32f4xx.h"
#include "log.h"
#include "sensirion_common.h"
#include "sensirion_i2c.h"
#include "sensirion_i2c_hal.h"
#include "sfm_sf06_i2c.h"
#include "system_gd32f4xx.h"

static const char *const gSfm3119Tag = "sfm3119";
static SFM3119_Result gSfm3119Results[SFM3119_SENSOR_NUM];
static int16_t gSfm3119FlowScale[SFM3119_SENSOR_NUM];
static int16_t gSfm3119FlowOffset[SFM3119_SENSOR_NUM];
static int16_t gSfm3119LastError[SFM3119_SENSOR_NUM];
static uint8_t gSfm3119Ready[SFM3119_SENSOR_NUM];
static uint8_t gSfm3119AirBuffer[SFM3119_FULL_FRAME_LENGTH];
static uint8_t gSfm3119O2Buffer[SFM3119_FULL_FRAME_LENGTH];
static uint8_t gSfm3119AirReadLength = 0U;
static uint8_t gSfm3119AirReadPending = 0U;
static uint8_t gSfm3119ReadErrorCount = 0U;
static uint32_t gSfm3119ProcessCycle = 0U;
static uint32_t gSfm3119MaxReadCycles = 0U;
static uint32_t gSfm3119MaxProcessCycles = 0U;
static int8_t gSfm3119Status = SFM3119_ERROR_NOT_READY;

static uint8_t sfm3119GetAddress(eSfm3119SensorIndex sensorIndex) {
    return (sensorIndex == SFM3119_AIR_INDEX) ? SFM3119_AIR_ADDR : SFM3119_O2_ADDR;
}

static uint8_t sfm3119GetBus(eSfm3119SensorIndex sensorIndex) {
    return (sensorIndex == SFM3119_AIR_INDEX) ? SENSIRION_I2C_HAL_BUS_AIR : SENSIRION_I2C_HAL_BUS_O2;
}

static int8_t sfm3119Select(eSfm3119SensorIndex sensorIndex) {
    if ((uint32_t)sensorIndex >= (uint32_t)SFM3119_SENSOR_NUM) {
        return SFM3119_ERROR_INVALID_INDEX;
    }
    if (sensirion_i2c_hal_select_bus(sfm3119GetBus(sensorIndex)) != SENSIRION_I2C_HAL_STATUS_OK) {
        return SFM3119_ERROR_IO;
    }
    sfm_sf06_init(sfm3119GetAddress(sensorIndex));
    return SFM3119_STATUS_OK;
}

static int8_t sfm3119MeasurementDecode(eSfm3119SensorIndex sensorIndex, const uint8_t *data, uint8_t length) {
    SFM3119_Result *lResult = &gSfm3119Results[sensorIndex];
    uint8_t lIndex;

    for (lIndex = 0U; lIndex < length; lIndex += SFM3119_FLOW_FRAME_LENGTH) {
        if (sensirion_i2c_check_crc(&data[lIndex], 2U, data[lIndex + 2U]) != 0) {
            gSfm3119LastError[sensorIndex] = CRC_ERROR;
            return SFM3119_ERROR_IO;
        }
    }
    lResult->flow_raw = sensirion_common_bytes_to_int16_t(&data[0]);
    lResult->flow_slm = ((float)lResult->flow_raw - (float)gSfm3119FlowOffset[sensorIndex]) /
                        (float)gSfm3119FlowScale[sensorIndex];
    if (length == SFM3119_FULL_FRAME_LENGTH) {
        lResult->temperature_raw = sensirion_common_bytes_to_int16_t(&data[3]);
        lResult->status = sensirion_common_bytes_to_uint16_t(&data[6]);
        lResult->temperature_degC = (float)lResult->temperature_raw / SFM3119_TEMPERATURE_SCALE;
    }
    gSfm3119LastError[sensorIndex] = 0;
    return SFM3119_STATUS_OK;
}

static int8_t sfm3119MeasurementReadSync(eSfm3119SensorIndex sensorIndex, uint8_t *data, uint8_t length) {
    int8_t lStatus;

    if (sfm3119Select(sensorIndex) != SFM3119_STATUS_OK) {
        return SFM3119_ERROR_IO;
    }
    lStatus = sensirion_i2c_hal_read(sfm3119GetAddress(sensorIndex), data, length);
    if (lStatus != SENSIRION_I2C_HAL_STATUS_OK) {
        gSfm3119LastError[sensorIndex] = lStatus;
        return SFM3119_ERROR_IO;
    }
    return sfm3119MeasurementDecode(sensorIndex, data, length);
}

static int8_t sfm3119AirReadFinish(void) {
    int8_t lStatus;

    if (gSfm3119AirReadPending == 0U) {
        return SFM3119_STATUS_OK;
    }
    lStatus = sensirion_i2c_hal_read_async_poll();
    if (lStatus == SENSIRION_I2C_HAL_STATUS_PENDING) {
        return SFM3119_STATUS_PENDING;
    }
    gSfm3119AirReadPending = 0U;
    if (lStatus != SENSIRION_I2C_HAL_STATUS_OK) {
        gSfm3119LastError[SFM3119_AIR_INDEX] = lStatus;
        return SFM3119_ERROR_IO;
    }
    return sfm3119MeasurementDecode(SFM3119_AIR_INDEX, gSfm3119AirBuffer, gSfm3119AirReadLength);
}

static void sfm3119AirReadAbort(void) {
    if (gSfm3119AirReadPending != 0U) {
        sensirion_i2c_hal_read_async_abort();
        gSfm3119AirReadPending = 0U;
    }
}

static int8_t sfm3119InitSensor(eSfm3119SensorIndex sensorIndex) {
    uint16_t lFlowUnit;
    uint16_t lCommand;
    int16_t lError;

    if (sfm3119Select(sensorIndex) != SFM3119_STATUS_OK) {
        gSfm3119LastError[sensorIndex] = SFM3119_ERROR_INVALID_INDEX;
        return SFM3119_ERROR_IO;
    }
    (void)sfm_sf06_stop_continuous_measurement();
    sensirion_i2c_hal_sleep_usec(1000U);
    lError = sfm_sf06_read_product_identifier(
        &gSfm3119Results[sensorIndex].product_identifier,
        gSfm3119Results[sensorIndex].serial_number,
        (uint16_t)sizeof(gSfm3119Results[sensorIndex].serial_number));
    if (lError != 0) {
        gSfm3119LastError[sensorIndex] = lError;
        return SFM3119_ERROR_IO;
    }

    lCommand = (sensorIndex == SFM3119_AIR_INDEX)
                   ? SFM_SF06_START_AIR_CONTINUOUS_MEASUREMENT_CMD_ID
                   : SFM_SF06_START_O2_CONTINUOUS_MEASUREMENT_CMD_ID;
    lError = sfm_sf06_read_scale_offset_unit(
        lCommand,
        &gSfm3119FlowScale[sensorIndex],
        &gSfm3119FlowOffset[sensorIndex],
        &lFlowUnit);
    if ((lError != 0) || (gSfm3119FlowScale[sensorIndex] == 0)) {
        gSfm3119LastError[sensorIndex] = (lError != 0) ? lError : SFM3119_ERROR_IO;
        return SFM3119_ERROR_IO;
    }

    lError = (sensorIndex == SFM3119_AIR_INDEX)
                 ? ll_sfm_sf06_start_air_continuous_measurement()
                 : ll_sfm_sf06_start_o2_continuous_measurement();
    if (lError != 0) {
        gSfm3119LastError[sensorIndex] = lError;
        return SFM3119_ERROR_IO;
    }
    gSfm3119LastError[sensorIndex] = 0;
    gSfm3119Ready[sensorIndex] = 1U;
    return SFM3119_STATUS_OK;
}

int8_t sfm3119Init(void) {
    const SFM3119_Result *lAirResult;
    const SFM3119_Result *lO2Result;
    int8_t lAirStatus;
    int8_t lO2Status;

    sfm3119AirReadAbort();
    sensirion_i2c_hal_init();
    gSfm3119Ready[SFM3119_AIR_INDEX] = 0U;
    gSfm3119Ready[SFM3119_O2_INDEX] = 0U;
    gSfm3119AirReadPending = 0U;
    gSfm3119ReadErrorCount = 0U;
    gSfm3119LastError[SFM3119_AIR_INDEX] = 0;
    gSfm3119LastError[SFM3119_O2_INDEX] = 0;
    lAirStatus = sfm3119InitSensor(SFM3119_AIR_INDEX);
    if (lAirStatus != SFM3119_STATUS_OK) {
        sensirion_i2c_hal_sleep_usec(SFM3119_INIT_RETRY_DELAY_US);
        lAirStatus = sfm3119InitSensor(SFM3119_AIR_INDEX);
    }
    lO2Status = sfm3119InitSensor(SFM3119_O2_INDEX);
    if (lO2Status != SFM3119_STATUS_OK) {
        sensirion_i2c_hal_sleep_usec(SFM3119_INIT_RETRY_DELAY_US);
        lO2Status = sfm3119InitSensor(SFM3119_O2_INDEX);
    }
    if (lAirStatus != SFM3119_STATUS_OK) {
        gSfm3119Status = lAirStatus;
    } else {
        gSfm3119Status = lO2Status;
    }

    if (gSfm3119Status != SFM3119_STATUS_OK) {
        LOG_E(gSfm3119Tag, "init failed status=%d air_err=%d o2_err=%d",
              (int)gSfm3119Status,
              (int)sfm3119GetLastError(SFM3119_AIR_INDEX),
              (int)sfm3119GetLastError(SFM3119_O2_INDEX));
        return gSfm3119Status;
    }

    /* SFM3119 NACKs read requests until its first result is ready. */
    sensirion_i2c_hal_sleep_usec(SFM3119_FIRST_MEASUREMENT_DELAY_US);

    lAirResult = sfm3119GetResult(SFM3119_AIR_INDEX);
    lO2Result = sfm3119GetResult(SFM3119_O2_INDEX);
    LOG_I(gSfm3119Tag,
          "ready air_pid=0x%08lX air_sn=%02X%02X%02X%02X%02X%02X%02X%02X",
          (unsigned long)lAirResult->product_identifier,
          lAirResult->serial_number[0], lAirResult->serial_number[1],
          lAirResult->serial_number[2], lAirResult->serial_number[3],
          lAirResult->serial_number[4], lAirResult->serial_number[5],
          lAirResult->serial_number[6], lAirResult->serial_number[7]);
    LOG_I(gSfm3119Tag,
          "ready o2_pid=0x%08lX o2_sn=%02X%02X%02X%02X%02X%02X%02X%02X",
          (unsigned long)lO2Result->product_identifier,
          lO2Result->serial_number[0], lO2Result->serial_number[1],
          lO2Result->serial_number[2], lO2Result->serial_number[3],
          lO2Result->serial_number[4], lO2Result->serial_number[5],
          lO2Result->serial_number[6], lO2Result->serial_number[7]);
    gSfm3119ProcessCycle = 0U;
    return SFM3119_STATUS_OK;
}

int8_t sfm3119Read(eSfm3119SensorIndex sensorIndex) {
    if ((uint32_t)sensorIndex >= (uint32_t)SFM3119_SENSOR_NUM) {
        return SFM3119_ERROR_INVALID_INDEX;
    }
    if (gSfm3119Ready[sensorIndex] == 0U) {
        return SFM3119_ERROR_NOT_READY;
    }
    return sfm3119MeasurementReadSync(sensorIndex,
                                      (sensorIndex == SFM3119_AIR_INDEX) ? gSfm3119AirBuffer : gSfm3119O2Buffer,
                                      SFM3119_FULL_FRAME_LENGTH);
}

int8_t sfm3119ReadAll(void) {
    uint8_t lReadLength;
    int8_t lAirStatus;
    int8_t lO2Status;

    lAirStatus = sfm3119AirReadFinish();
    if (lAirStatus == SFM3119_STATUS_PENDING) {
        return SFM3119_STATUS_OK;
    }
    if (lAirStatus != SFM3119_STATUS_OK) {
        return lAirStatus;
    }
    lReadLength = ((gSfm3119ProcessCycle % SFM3119_AUX_UPDATE_CYCLES) == 0U)
                      ? SFM3119_FULL_FRAME_LENGTH
                      : SFM3119_FLOW_FRAME_LENGTH;
    if (sfm3119Select(SFM3119_AIR_INDEX) != SFM3119_STATUS_OK) {
        return SFM3119_ERROR_IO;
    }
    lAirStatus = sensirion_i2c_hal_read_async_start(SFM3119_AIR_ADDR,
                                                    gSfm3119AirBuffer,
                                                    lReadLength);
    if (lAirStatus != SENSIRION_I2C_HAL_STATUS_OK) {
        gSfm3119LastError[SFM3119_AIR_INDEX] = lAirStatus;
        return SFM3119_ERROR_IO;
    }
    gSfm3119AirReadLength = lReadLength;
    gSfm3119AirReadPending = 1U;
    lO2Status = sfm3119MeasurementReadSync(SFM3119_O2_INDEX, gSfm3119O2Buffer, lReadLength);
    lAirStatus = sfm3119AirReadFinish();

    if ((lO2Status != SFM3119_STATUS_OK) &&
        (lAirStatus == SFM3119_STATUS_PENDING)) {
        sfm3119AirReadAbort();
    }

    if ((lAirStatus != SFM3119_STATUS_OK) &&
        (lAirStatus != SFM3119_STATUS_PENDING)) {
        return lAirStatus;
    }
    return lO2Status;
}

int8_t sfm3119Process(void) {
    uint32_t lReadCycles;
    uint32_t lReadUs;
    uint32_t lMaxReadUs;
    uint32_t lMaxProcessUs;
    uint32_t lProcessCycles;
    uint32_t lStartCycles = DWT->CYCCNT;

    if (gSfm3119Status == SFM3119_STATUS_OK) {
        gSfm3119Status = sfm3119ReadAll();
        lReadCycles = (uint32_t)(DWT->CYCCNT - lStartCycles);
        if (lReadCycles > gSfm3119MaxReadCycles) {
            gSfm3119MaxReadCycles = lReadCycles;
        }
        if (gSfm3119Status == SFM3119_STATUS_OK) {
            gSfm3119ReadErrorCount = 0U;
        } else if (++gSfm3119ReadErrorCount < SFM3119_READ_ERROR_LIMIT) {
            /* A measurement read may NACK briefly while the next sample is prepared. */
            gSfm3119Status = SFM3119_STATUS_OK;
        } else {
            gSfm3119ReadErrorCount = 0U;
            LOG_E(gSfm3119Tag, "read failed status=%d air_err=%d o2_err=%d",
                  (int)gSfm3119Status,
                  (int)sfm3119GetLastError(SFM3119_AIR_INDEX),
                  (int)sfm3119GetLastError(SFM3119_O2_INDEX));
        }
        if ((gSfm3119Status == SFM3119_STATUS_OK) &&
            ((gSfm3119ProcessCycle % SFM3119_PROCESS_LOG_CYCLES) == 0U)) {
            lReadUs = lReadCycles / (SystemCoreClock / 1000000U);
            lMaxReadUs = gSfm3119MaxReadCycles / (SystemCoreClock / 1000000U);
            lMaxProcessUs = gSfm3119MaxProcessCycles / (SystemCoreClock / 1000000U);
            // LOG_I(gSfm3119Tag, "timing read_us=%lu max_read_us=%lu max_process_us=%lu",
            //       (unsigned long)lReadUs, (unsigned long)lMaxReadUs,
            //       (unsigned long)lMaxProcessUs);
            gSfm3119MaxReadCycles = 0U;
            gSfm3119MaxProcessCycles = 0U;
        }
    } else if ((gSfm3119ProcessCycle % SFM3119_PROCESS_LOG_CYCLES) == 0U) {
        gSfm3119Status = sfm3119Init();
    }

    gSfm3119ProcessCycle++;
    lProcessCycles = (uint32_t)(DWT->CYCCNT - lStartCycles);
    if (lProcessCycles > gSfm3119MaxProcessCycles) {
        gSfm3119MaxProcessCycles = lProcessCycles;
    }
    return gSfm3119Status;
}

const SFM3119_Result* sfm3119GetResult(eSfm3119SensorIndex sensorIndex) {
    if ((uint32_t)sensorIndex >= (uint32_t)SFM3119_SENSOR_NUM) {
        return NULL;
    }
    return &gSfm3119Results[sensorIndex];
}

int16_t sfm3119GetLastError(eSfm3119SensorIndex sensorIndex) {
    if ((uint32_t)sensorIndex >= (uint32_t)SFM3119_SENSOR_NUM) {
        return SFM3119_ERROR_INVALID_INDEX;
    }
    return gSfm3119LastError[sensorIndex];
}

/**************************End of file********************************/
