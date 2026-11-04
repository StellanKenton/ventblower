/************************************************************************************
* @file     : adc.h
* @brief    : Board ADC phase current sampling interface.
* @details  : Defines phase current indexes, the current loop hook and the initialization entry point.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef USER_BSP_ADC_H
#define USER_BSP_ADC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Phase currents use one inserted channel per ADC so U/V/W are sampled together. */
typedef enum eAdcPhase {
    ADC_PHASE_U = 0,    /* PC5, ADC2 IN15 */
    ADC_PHASE_V,        /* PB0, ADC0 IN8 */
    ADC_PHASE_W,        /* PB1, ADC1 IN9 */
    ADC_PHASE_COUNT
} eAdcPhase;

/* Callback run in ADC_IRQHandler after each PWM-triggered current sample. */
typedef void (*pfAdcCurrentCallback)(void);

#define ADC_CURRENT_IRQ_PRIORITY    1U

extern volatile uint16_t gAdcPhaseRaw[ADC_PHASE_COUNT];

/**
 * @brief Arm TIMER0 CH3 triggered phase current sampling.
 * @note Current samples start once pwmInit() runs TIMER0.
 */
void adcInit(void);

/**
 * @brief Register the current loop hook.
 * @param callback Hook called from ADC_IRQHandler, or NULL to detach.
 * @note The hook runs above configMAX_SYSCALL_INTERRUPT_PRIORITY and must not call RTOS APIs.
 */
void adcCurrentCallbackSet(pfAdcCurrentCallback callback);

/** @brief ADC0/1/2 shared interrupt entry point. */
void ADC_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_BSP_ADC_H */
/**************************End of file********************************/
