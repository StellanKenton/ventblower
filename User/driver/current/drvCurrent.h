/************************************************************************************
* @file     : drvCurrent.h
* @brief    : Phase current monitor.
* @details  : Calibrates the zero offsets, converts ADC samples to amps and trips the
*             PWM outputs on overcurrent. Runs in the ADC current sample interrupt.
* @author   : Stellan
* @date     :
* @version  :
* @copyright: Copyright (c) 2026 ResMed. All rights reserved.
***********************************************************************************/
#ifndef DRV_CURRENT_H
#define DRV_CURRENT_H

#include <stdint.h>

#include "adc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Status reason codes. */
#define DRV_CURRENT_STATUS_OK               1
#define DRV_CURRENT_STATUS_INVALID_PARAM    (-1)
#define DRV_CURRENT_STATUS_BUSY             (-2)

/* Sense chain per the board schematic: 10 mOhm low-side shunt, gain 10, 1.65 V offset. */
#define DRV_CURRENT_ADC_VREF_V              3.3f
#define DRV_CURRENT_ADC_FULL_SCALE          4096.0f
#define DRV_CURRENT_SHUNT_OHM               0.010f
#define DRV_CURRENT_AMP_GAIN                10.0f
/* Negative: current flowing from the bridge to the motor pulls the amplifier below 1.65 V. */
#define DRV_CURRENT_A_PER_COUNT             \
    (-(DRV_CURRENT_ADC_VREF_V / DRV_CURRENT_ADC_FULL_SCALE) / (DRV_CURRENT_SHUNT_OHM * DRV_CURRENT_AMP_GAIN))

/* A calibrated offset outside nominal +/- window means a broken channel or a live current. */
#define DRV_CURRENT_OFFSET_NOMINAL          2048.0f
#define DRV_CURRENT_OFFSET_WINDOW           200.0f
/* 10 ms at 20 kHz: VCC_REF needs about 3 ms to settle after power-up. */
#define DRV_CURRENT_CAL_SKIP_SAMPLES        200U
#define DRV_CURRENT_CAL_SAMPLES             1024U
/* Each phase is sampled in two of every three rotating periods. */
#define DRV_CURRENT_CAL_PERIODS             ((DRV_CURRENT_CAL_SAMPLES * ADC_PHASE_COUNT) / ADC_SAMPLED_PHASE_COUNT)

/* Per-sample trip level in amps. Set for the motor's 3 A peak; review after the sense rework. */
#define DRV_CURRENT_TRIP_A                  6.0f

typedef enum eDrvCurrentState {
    DRV_CURRENT_STATE_CALIBRATING = 0,
    DRV_CURRENT_STATE_READY,
    DRV_CURRENT_STATE_CAL_FAILED,
    DRV_CURRENT_STATE_OVERCURRENT
} eDrvCurrentState;

/* Indexed by eAdcChannel phase slots: ADC_CH_IU, ADC_CH_IV, ADC_CH_IW. */
typedef struct stDrvCurrentData {
    eDrvCurrentState state;
    float current[ADC_PHASE_COUNT];     /* Latest value in amps, positive into the motor; one phase is derived. */
    float peak[ADC_PHASE_COUNT];        /* Largest magnitude in amps since calibration. */
    float offset[ADC_PHASE_COUNT];      /* Zero-current ADC reading in counts. */
} stDrvCurrentData;

/**
 * @brief Attach to the ADC current interrupt and start the boot calibration.
 * @note Call after adcInit() and before pwmInit(). Sampling starts with TIMER0.
 */
void drvCurrentInit(void);

/**
 * @brief Force the gates off, recalibrate the offsets and clear a latched fault.
 * @return DRV_CURRENT_STATUS_OK, or DRV_CURRENT_STATUS_BUSY while already calibrating.
 * @note Task context. Takes about 87 ms at 20 kHz. PWM stays off unless the state is READY.
 */
int8_t drvCurrentCalibrateStart(void);

/**
 * @brief Copy the latest monitor values.
 * @param data Destination snapshot.
 * @return DRV_CURRENT_STATUS_OK or DRV_CURRENT_STATUS_INVALID_PARAM.
 * @note Task context. Each field is atomic, but phases may come from adjacent PWM periods.
 */
int8_t drvCurrentRead(stDrvCurrentData *data);

#ifdef __cplusplus
}
#endif

#endif /* DRV_CURRENT_H */
/**************************End of file********************************/
