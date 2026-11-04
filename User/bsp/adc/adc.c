/************************************************************************************
* @file     : adc.c
* @brief    : Board ADC phase current sampling driver.
* @details  : ADC0/1/2 inserted groups sample U/V/W together on the TIMER0 CH3 trigger.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "adc.h"

#include <stddef.h>

#include "gd32f4xx.h"
#include "gd32f4xx_adc.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"

volatile uint16_t gAdcPhaseRaw[ADC_PHASE_COUNT];

static pfAdcCurrentCallback gAdcCurrentCallback = NULL;

static void adcGpioInit(void)
{
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);

    /* PB0/PB1/PC5 are the V/W/U phase currents. */
    gpio_mode_set(GPIOB, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_1);
    gpio_mode_set(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_5);
}

static void adcCurrentInit(void)
{
    static const uint32_t lAdcList[ADC_PHASE_COUNT] = {ADC2, ADC0, ADC1};
    static const uint8_t lChannelList[ADC_PHASE_COUNT] = {ADC_CHANNEL_15, ADC_CHANNEL_8, ADC_CHANNEL_9};
    uint8_t lPhase;

    rcu_periph_clock_enable(RCU_ADC0);
    rcu_periph_clock_enable(RCU_ADC1);
    rcu_periph_clock_enable(RCU_ADC2);
    adc_deinit();
    adc_sync_mode_config(ADC_SYNC_MODE_INDEPENDENT);
    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);

    /* Same trigger and sample time on every ADC keeps the three conversions aligned. */
    for (lPhase = 0U; lPhase < ADC_PHASE_COUNT; ++lPhase) {
        uint32_t lAdc = lAdcList[lPhase];

        adc_resolution_config(lAdc, ADC_RESOLUTION_12B);
        adc_data_alignment_config(lAdc, ADC_DATAALIGN_RIGHT);
        adc_channel_length_config(lAdc, ADC_INSERTED_CHANNEL, 1U);
        adc_inserted_channel_config(lAdc, 0U, lChannelList[lPhase], ADC_SAMPLETIME_15);
        adc_external_trigger_source_config(lAdc, ADC_INSERTED_CHANNEL, ADC_EXTTRIG_INSERTED_T0_CH3);
        adc_external_trigger_config(lAdc, ADC_INSERTED_CHANNEL, EXTERNAL_TRIGGER_RISING);
        adc_enable(lAdc);
        adc_calibration_enable(lAdc);
    }

    adc_interrupt_flag_clear(ADC0, ADC_INT_FLAG_EOIC);
    adc_interrupt_enable(ADC0, ADC_INT_EOIC);
    NVIC_SetPriority(ADC_IRQn, ADC_CURRENT_IRQ_PRIORITY);
    NVIC_EnableIRQ(ADC_IRQn);
}

void adcInit(void)
{
    adcGpioInit();
    adcCurrentInit();
}

void adcCurrentCallbackSet(pfAdcCurrentCallback callback)
{
    gAdcCurrentCallback = callback;
}

/** ADC0 EOIC fires once per PWM period; ADC1/ADC2 finish on the same ADC clock edge. */
void ADC_IRQHandler(void)
{
    if (adc_interrupt_flag_get(ADC0, ADC_INT_FLAG_EOIC) == RESET) {
        return;
    }
    adc_interrupt_flag_clear(ADC0, ADC_INT_FLAG_EOIC);

    gAdcPhaseRaw[ADC_PHASE_U] = adc_inserted_data_read(ADC2, ADC_INSERTED_CHANNEL_0);
    gAdcPhaseRaw[ADC_PHASE_V] = adc_inserted_data_read(ADC0, ADC_INSERTED_CHANNEL_0);
    gAdcPhaseRaw[ADC_PHASE_W] = adc_inserted_data_read(ADC1, ADC_INSERTED_CHANNEL_0);
    if (gAdcCurrentCallback != NULL) {
        gAdcCurrentCallback();
    }
}

/**************************End of file********************************/
