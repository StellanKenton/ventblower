/************************************************************************************
* @file     : drvCurrent.c
* @brief    : Phase current monitor.
* @details  : All sample processing runs in the ADC interrupt, which is above the RTOS
*             mask level and must not call RTOS or LOG APIs.
* @author   : Stellan
* @date     :
* @version  :
* @copyright: Copyright (c) 2026 ResMed. All rights reserved.
***********************************************************************************/
#include "drvCurrent.h"

#include <stdbool.h>
#include <stddef.h>

#include "pwm.h"

static volatile eDrvCurrentState gDrvCurrentState = DRV_CURRENT_STATE_CALIBRATING;
static volatile float gDrvCurrentValue[ADC_PHASE_COUNT];
static volatile float gDrvCurrentPeak[ADC_PHASE_COUNT];
static volatile float gDrvCurrentOffset[ADC_PHASE_COUNT];
/* Only the interrupt touches these, so a recalibration request cannot race them. */
static uint32_t gDrvCurrentCalSum[ADC_PHASE_COUNT];
static uint32_t gDrvCurrentCalCount;

static float drvCurrentAbs(float value) {
    return (value < 0.0f) ? -value : value;
}

/** Accumulate one sample set and publish the offsets once the window is complete. */
static void drvCurrentCalibrateStep(eAdcChannel skipPhase) {
    uint8_t lPhase;
    bool lValid = true;

    gDrvCurrentCalCount++;
    if (gDrvCurrentCalCount <= DRV_CURRENT_CAL_SKIP_SAMPLES) {
        return;
    }
    for (lPhase = 0U; lPhase < ADC_PHASE_COUNT; lPhase++) {
        if (lPhase != (uint8_t)skipPhase) {
            gDrvCurrentCalSum[lPhase] += gAdcRaw[lPhase];
        }
    }
    if (gDrvCurrentCalCount < (DRV_CURRENT_CAL_SKIP_SAMPLES + DRV_CURRENT_CAL_PERIODS)) {
        return;
    }

    for (lPhase = 0U; lPhase < ADC_PHASE_COUNT; lPhase++) {
        float lOffset = (float)gDrvCurrentCalSum[lPhase] / (float)DRV_CURRENT_CAL_SAMPLES;

        gDrvCurrentOffset[lPhase] = lOffset;
        lValid = lValid && (drvCurrentAbs(lOffset - DRV_CURRENT_OFFSET_NOMINAL) <= DRV_CURRENT_OFFSET_WINDOW);
        gDrvCurrentPeak[lPhase] = 0.0f;
        gDrvCurrentCalSum[lPhase] = 0U;
    }
    gDrvCurrentCalCount = 0U;
    gDrvCurrentState = lValid ? DRV_CURRENT_STATE_READY : DRV_CURRENT_STATE_CAL_FAILED;
}

/** Publish one phase current, track its peak and latch overcurrent. */
static void drvCurrentPhaseUpdate(uint8_t phase, float current) {
    float lMagnitude = drvCurrentAbs(current);

    gDrvCurrentValue[phase] = current;
    if (lMagnitude > gDrvCurrentPeak[phase]) {
        gDrvCurrentPeak[phase] = lMagnitude;
    }
    if (lMagnitude > DRV_CURRENT_TRIP_A) {
        gDrvCurrentState = DRV_CURRENT_STATE_OVERCURRENT;
    }
}

/** Convert the two sampled phases and derive the third, so all phases are checked. */
static void drvCurrentMonitorStep(eAdcChannel skipPhase) {
    uint8_t lPhase;
    float lSum = 0.0f;

    for (lPhase = 0U; lPhase < ADC_PHASE_COUNT; lPhase++) {
        if (lPhase != (uint8_t)skipPhase) {
            float lCurrent = ((float)gAdcRaw[lPhase] - gDrvCurrentOffset[lPhase]) * DRV_CURRENT_A_PER_COUNT;

            drvCurrentPhaseUpdate(lPhase, lCurrent);
            lSum += lCurrent;
        }
    }
    /* The motor neutral is floating, so the three phase currents sum to zero. */
    drvCurrentPhaseUpdate((uint8_t)skipPhase, -lSum);
}

static eAdcChannel drvCurrentIsr(eAdcChannel skipPhase) {
    if (gDrvCurrentState == DRV_CURRENT_STATE_CALIBRATING) {
        drvCurrentCalibrateStep(skipPhase);
    } else if (gDrvCurrentState == DRV_CURRENT_STATE_READY) {
        drvCurrentMonitorStep(skipPhase);
    }

    /* Re-asserted every period so a late pwmOutputSet(true) cannot override a fault. */
    if (gDrvCurrentState != DRV_CURRENT_STATE_READY) {
        pwmOutputSet(false);
    }

    /* Rotate so every phase is measured and each gets equal calibration samples. */
    return (eAdcChannel)(((uint8_t)skipPhase + 1U) % ADC_PHASE_COUNT);
}

void drvCurrentInit(void) {
    adcCurrentCallbackSet(drvCurrentIsr);
}

int8_t drvCurrentCalibrateStart(void) {
    if (gDrvCurrentState == DRV_CURRENT_STATE_CALIBRATING) {
        return DRV_CURRENT_STATUS_BUSY;
    }

    gDrvCurrentState = DRV_CURRENT_STATE_CALIBRATING;
    return DRV_CURRENT_STATUS_OK;
}

int8_t drvCurrentRead(stDrvCurrentData *data) {
    uint8_t lPhase;

    if (data == NULL) {
        return DRV_CURRENT_STATUS_INVALID_PARAM;
    }

    data->state = gDrvCurrentState;
    for (lPhase = 0U; lPhase < ADC_PHASE_COUNT; lPhase++) {
        data->current[lPhase] = gDrvCurrentValue[lPhase];
        data->peak[lPhase] = gDrvCurrentPeak[lPhase];
        data->offset[lPhase] = gDrvCurrentOffset[lPhase];
    }
    return DRV_CURRENT_STATUS_OK;
}

/**************************End of file********************************/
