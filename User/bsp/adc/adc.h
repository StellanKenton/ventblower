/************************************************************************************
* @file     : adc.h
* @brief    : Board ADC acquisition interface.
* @details  : Defines raw sample indexes, the current loop hook and the initialization entry point.
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

/* Raw sample slots: phase currents first, then the routine scan channels. */
typedef enum eAdcChannel {
    ADC_CH_IU = 0,      /* PC5, ADC2 IN15, inserted */
    ADC_CH_IV,          /* PB0, ADC0 IN8, inserted */
    ADC_CH_IW,          /* PB1, ADC1 IN9, inserted */
    ADC_CH_VBUS,        /* PA5, ADC1 IN5, routine */
    ADC_CH_NTC,         /* PC3, ADC1 IN13, routine */
    ADC_CH_COUNT
} eAdcChannel;

/* Callback run in ADC_IRQHandler after each PWM-triggered current sample. */
typedef void (*pfAdcCurrentCallback)(void);

#define ADC_CURRENT_IRQ_PRIORITY    1U
#define ADC_PHASE_COUNT             ((uint8_t)ADC_CH_VBUS)
#define ADC_ROUTINE_COUNT           ((uint8_t)(ADC_CH_COUNT - ADC_CH_VBUS))

extern volatile uint16_t gAdcRaw[ADC_CH_COUNT];

/**
 * @brief Start the VBUS/NTC routine scan and arm TIMER0 CH3 triggered phase current sampling.
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
