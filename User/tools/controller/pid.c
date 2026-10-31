/***********************************************************************************
* @file     : pid.c
* @brief    : Lightweight PID controller implementation.
* @details  : The controller includes output limiting and conditional anti-windup.
* @author   :
* @date     : 2026-08-20
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/

#include "pid.h"

#include <float.h>
#include <stddef.h>

static uint8_t pidIsFinite(float value)
{
    return (uint8_t)((value >= -FLT_MAX) && (value <= FLT_MAX));
}

static float pidClamp(float value, float minimum, float maximum)
{
    if (value > maximum) {
        return maximum;
    }

    if (value < minimum) {
        return minimum;
    }

    return value;
}

static uint8_t pidHasValidTunings(float kp, float ki, float kd)
{
    return (uint8_t)((pidIsFinite(kp) != 0U) &&
                     (pidIsFinite(ki) != 0U) &&
                     (pidIsFinite(kd) != 0U));
}

static uint8_t pidHasValidLimits(float outputMin, float outputMax)
{
    return (uint8_t)((pidIsFinite(outputMin) != 0U) &&
                     (pidIsFinite(outputMax) != 0U) &&
                     (outputMin < outputMax));
}

int8_t pidInit(stPid *controller, float kp, float ki, float kd, float samplePeriod, float outputMin, float outputMax)
{
    if (controller == NULL) {
        return PID_ERROR_PARAM;
    }

    if ((pidHasValidTunings(kp, ki, kd) == 0U) ||
        (pidIsFinite(samplePeriod) == 0U) ||
        (samplePeriod <= 0.0f) ||
        (pidHasValidLimits(outputMin, outputMax) == 0U)) {
        controller->initialized = 0U;
        return PID_ERROR_CONFIG;
    }

    controller->kp = kp;
    controller->ki = ki;
    controller->kd = kd;
    controller->samplePeriod = samplePeriod;
    controller->outputMin = outputMin;
    controller->outputMax = outputMax;
    controller->initialized = 1U;

    return pidReset(controller);
}

int8_t pidReset(stPid *controller)
{
    if (controller == NULL) {
        return PID_ERROR_PARAM;
    }

    if (controller->initialized == 0U) {
        return PID_ERROR_STATE;
    }

    controller->integral = 0.0f;
    controller->previousMeasurement = 0.0f;
    controller->output = pidClamp(0.0f, controller->outputMin, controller->outputMax);
    controller->hasPreviousMeasurement = 0U;

    return PID_STATUS_OK;
}

int8_t pidSetTunings(stPid *controller, float kp, float ki, float kd)
{
    if (controller == NULL) {
        return PID_ERROR_PARAM;
    }

    if (controller->initialized == 0U) {
        return PID_ERROR_STATE;
    }

    if (pidHasValidTunings(kp, ki, kd) == 0U) {
        return PID_ERROR_CONFIG;
    }

    controller->kp = kp;
    controller->ki = ki;
    controller->kd = kd;

    return PID_STATUS_OK;
}

int8_t pidSetOutputLimits(stPid *controller, float outputMin, float outputMax)
{
    if (controller == NULL) {
        return PID_ERROR_PARAM;
    }

    if (controller->initialized == 0U) {
        return PID_ERROR_STATE;
    }

    if (pidHasValidLimits(outputMin, outputMax) == 0U) {
        return PID_ERROR_CONFIG;
    }

    controller->outputMin = outputMin;
    controller->outputMax = outputMax;
    controller->integral = pidClamp(controller->integral, outputMin, outputMax);
    controller->output = pidClamp(controller->output, outputMin, outputMax);

    return PID_STATUS_OK;
}

int8_t pidTrackOutput(stPid *controller, float setpoint, float measurement, float desiredOutput)
{
    float lError;
    float lProportional;
    float lIntegralStep;

    if (controller == NULL) {
        return PID_ERROR_PARAM;
    }

    if (controller->initialized == 0U) {
        return PID_ERROR_STATE;
    }

    if ((pidIsFinite(setpoint) == 0U) ||
        (pidIsFinite(measurement) == 0U) ||
        (pidIsFinite(desiredOutput) == 0U)) {
        return PID_ERROR_PARAM;
    }

    lError = setpoint - measurement;
    lProportional = controller->kp * lError;
    lIntegralStep = controller->ki * controller->samplePeriod * lError;
    desiredOutput = pidClamp(desiredOutput,
                             controller->outputMin,
                             controller->outputMax);

    /* Cancel the next integral step and suppress derivative kick on entry. */
    controller->integral = pidClamp(desiredOutput - lProportional - lIntegralStep,
                                    controller->outputMin,
                                    controller->outputMax);
    controller->previousMeasurement = measurement;
    controller->hasPreviousMeasurement = 1U;
    controller->output = desiredOutput;

    return PID_STATUS_OK;
}

int8_t pidUpdate(stPid *controller, float setpoint, float measurement, float *output)
{
    float lError;
    float lProportional;
    float lIntegralStep;
    float lIntegralCandidate;
    float lDerivative;
    float lUnclampedOutput;

    if ((controller == NULL) || (output == NULL)) {
        return PID_ERROR_PARAM;
    }

    if (controller->initialized == 0U) {
        return PID_ERROR_STATE;
    }

    if ((pidIsFinite(setpoint) == 0U) || (pidIsFinite(measurement) == 0U)) {
        return PID_ERROR_PARAM;
    }

    lError = setpoint - measurement;
    lProportional = controller->kp * lError;
    lIntegralStep = controller->ki * controller->samplePeriod * lError;
    lIntegralCandidate = pidClamp(controller->integral + lIntegralStep,
                                  controller->outputMin,
                                  controller->outputMax);
    lDerivative = 0.0f;

    /* Derivative on measurement avoids a kick when the setpoint changes. */
    if (controller->hasPreviousMeasurement != 0U) {
        lDerivative = -controller->kd *
                      (measurement - controller->previousMeasurement) /
                      controller->samplePeriod;
    }

    lUnclampedOutput = lProportional + lIntegralCandidate + lDerivative;

    /* Integrate only when it does not push a saturated output farther away. */
    if (((lUnclampedOutput <= controller->outputMax) &&
         (lUnclampedOutput >= controller->outputMin)) ||
        ((lUnclampedOutput > controller->outputMax) && (lIntegralStep < 0.0f)) ||
        ((lUnclampedOutput < controller->outputMin) && (lIntegralStep > 0.0f))) {
        controller->integral = lIntegralCandidate;
    }

    controller->output = pidClamp(lProportional + controller->integral + lDerivative,
                                  controller->outputMin,
                                  controller->outputMax);
    controller->previousMeasurement = measurement;
    controller->hasPreviousMeasurement = 1U;
    *output = controller->output;

    return PID_STATUS_OK;
}

/**************************End of file********************************/
