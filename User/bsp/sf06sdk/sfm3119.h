/************************************************************************************
* @file     : sfm3119.h
* @brief    : Dual SFM3119 flow sensor interface.
* @details  : Exposes air and oxygen results selected by sensor index.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef USER_BSP_SF06SDK_SFM3119_H
#define USER_BSP_SF06SDK_SFM3119_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SFM3119_STATUS_OK                 ((int8_t)1)
#define SFM3119_STATUS_PENDING            ((int8_t)2)
#define SFM3119_ERROR_INVALID_INDEX       ((int8_t)-1)
#define SFM3119_ERROR_NOT_READY           ((int8_t)-2)
#define SFM3119_ERROR_IO                  ((int8_t)-3)
#define SFM3119_AIR_ADDR                  0x29U
#define SFM3119_O2_ADDR                   0x29U
#define SFM3119_TEMPERATURE_SCALE         200.0f
#define SFM3119_PROCESS_LOG_CYCLES        500U
#define SFM3119_INIT_RETRY_DELAY_US        2000U
#define SFM3119_FIRST_MEASUREMENT_DELAY_US 12000U
#define SFM3119_FLOW_FRAME_LENGTH          3U
#define SFM3119_FULL_FRAME_LENGTH          9U
#define SFM3119_AUX_UPDATE_CYCLES          100U
#define SFM3119_READ_ERROR_LIMIT           5U

typedef enum eSfm3119SensorIndex {
    SFM3119_AIR_INDEX = 0,
    SFM3119_O2_INDEX,
    SFM3119_SENSOR_NUM
} eSfm3119SensorIndex;

typedef struct stSfm3119Result {
    int16_t flow_raw;
    int16_t temperature_raw;
    uint16_t status;
    uint32_t product_identifier;
    uint8_t serial_number[8];
    float flow_slm;
    float temperature_degC;
} SFM3119_Result;

/**
 * @brief Initialize both SFM3119 sensors and start continuous measurement.
 * @return SFM3119_STATUS_OK on success, otherwise a negative error code.
 * @note Call from task context. The driver is non-reentrant and should be owned
 * by one sensor task.
 */
int8_t sfm3119Init(void);

/**
 * @brief Refresh one cached sensor result.
 * @param sensorIndex Air or oxygen sensor index.
 * @return SFM3119_STATUS_OK on success, otherwise a negative error code.
 */
int8_t sfm3119Read(eSfm3119SensorIndex sensorIndex);

/**
 * @brief Refresh both cached sensor results.
 * @return SFM3119_STATUS_OK when both reads succeed, otherwise a negative error code.
 */
int8_t sfm3119ReadAll(void);

/**
 * @brief Process periodic reads, logging, and automatic recovery.
 * @return SFM3119_STATUS_OK on success, otherwise a negative error code.
 */
int8_t sfm3119Process(void);

/**
 * @brief Get the cached result selected by sensor index.
 * @param sensorIndex Air or oxygen sensor index.
 * @return Result pointer, or NULL when the index is invalid.
 */
const SFM3119_Result* sfm3119GetResult(eSfm3119SensorIndex sensorIndex);

/**
 * @brief Get the last underlying SDK or HAL error for one sensor.
 * @param sensorIndex Air or oxygen sensor index.
 * @return Zero after success, otherwise the underlying error code.
 */
int16_t sfm3119GetLastError(eSfm3119SensorIndex sensorIndex);

#ifdef __cplusplus
}
#endif

#endif /* USER_BSP_SF06SDK_SFM3119_H */
/**************************End of file********************************/
