/************************************************************************************
* @file     : pid.h
* @brief    : Lightweight PID controller public API.
* @details  : This module provides a fixed-period floating-point PID controller.
* @author   :
* @date     : 2026-08-20
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef PID_H
#define PID_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PID_STATUS_OK                 ((int8_t)1)
#define PID_ERROR_PARAM               ((int8_t)-1)
#define PID_ERROR_CONFIG              ((int8_t)-2)
#define PID_ERROR_STATE               ((int8_t)-3)

/** Fixed-period positional PID controller state. */
typedef struct stPid {
    float kp;
    float ki;
    float kd;
    float samplePeriod;
    float outputMin;
    float outputMax;
    float integral;
    float previousMeasurement;
    float output;
    uint8_t initialized;
    uint8_t hasPreviousMeasurement;
} stPid;

/** Initialize controller parameters and clear runtime state. */
int8_t pidInit(stPid *controller, float kp, float ki, float kd, float samplePeriod, float outputMin, float outputMax);

/** Clear integral, derivative history, and output while keeping configuration. */
int8_t pidReset(stPid *controller);

/** Update controller gains without clearing runtime state. */
int8_t pidSetTunings(stPid *controller, float kp, float ki, float kd);

/** Update output limits and clamp the current integral and output. */
int8_t pidSetOutputLimits(stPid *controller, float outputMin, float outputMax);

/** Track a desired output so the next PID update starts without an output step. */
int8_t pidTrackOutput(stPid *controller, float setpoint, float measurement, float desiredOutput);

/** Execute one PID update using the configured fixed sample period. */
int8_t pidUpdate(stPid *controller, float setpoint, float measurement, float *output);

#ifdef __cplusplus
}
#endif

#endif
/**************************End of file********************************/
