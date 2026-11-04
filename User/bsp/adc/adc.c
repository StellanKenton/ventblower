/************************************************************************************
* @file     : adc.c
* @brief    : Board ADC acquisition driver.
* @details  : ADC0/1/2 inserted groups sample U/V/W together on the TIMER0 CH3 trigger.
*             ADC1 routine scan with DMA1 circular mode samples VBUS and NTC.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "adc.h"

#include <stddef.h>

#include "gd32f4xx.h"
#include "gd32f4xx_adc.h"
#include "gd32f4xx_dma.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"

volatile uint16_t gAdcRaw[ADC_CH_COUNT];

static pfAdcCurrentCallback gAdcCurrentCallback = NULL;

static void adcGpioInit(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);

    /* PC5/PB0/PB1 are the U/V/W phase currents, PA5 is VBUS, PC3 is the NTC. */
    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_5);
    gpio_mode_set(GPIOB, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_1);
    gpio_mode_set(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_3 | GPIO_PIN_5);
}

static void adcRoutineInit(void)
{
    static const uint8_t lChannelList[ADC_ROUTINE_COUNT] = {ADC_CHANNEL_5, ADC_CHANNEL_13};
    dma_single_data_parameter_struct lDmaConfig;
    uint8_t lRank;

    rcu_periph_clock_enable(RCU_DMA1);
    dma_deinit(DMA1, DMA_CH2);
    dma_single_data_para_struct_init(&lDmaConfig);

    /* DMA fills the routine slots that follow the phase currents in gAdcRaw. */
    lDmaConfig.periph_addr = (uint32_t)&ADC_RDATA(ADC1);
    lDmaConfig.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    lDmaConfig.memory0_addr = (uint32_t)&gAdcRaw[ADC_CH_VBUS];
    lDmaConfig.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    lDmaConfig.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
    lDmaConfig.circular_mode = DMA_CIRCULAR_MODE_ENABLE;
    lDmaConfig.direction = DMA_PERIPH_TO_MEMORY;
    lDmaConfig.number = ADC_ROUTINE_COUNT;
    lDmaConfig.priority = DMA_PRIORITY_HIGH;

    dma_single_data_mode_init(DMA1, DMA_CH2, &lDmaConfig);
    /* ADC1 is mapped to DMA1 channel 2, subperipheral 1. */
    dma_channel_subperipheral_select(DMA1, DMA_CH2, DMA_SUBPERI1);
    dma_channel_enable(DMA1, DMA_CH2);

    adc_special_function_config(ADC1, ADC_SCAN_MODE, ENABLE);
    adc_special_function_config(ADC1, ADC_CONTINUOUS_MODE, ENABLE);
    adc_channel_length_config(ADC1, ADC_ROUTINE_CHANNEL, ADC_ROUTINE_COUNT);
    for (lRank = 0U; lRank < ADC_ROUTINE_COUNT; ++lRank) {
        adc_routine_channel_config(ADC1, lRank, lChannelList[lRank], ADC_SAMPLETIME_144);
    }
    adc_external_trigger_config(ADC1, ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_DISABLE);
    adc_dma_request_after_last_enable(ADC1);
    adc_dma_mode_enable(ADC1);
}

static void adcCurrentInit(void)
{
    static const uint32_t lAdcList[ADC_PHASE_COUNT] = {ADC2, ADC0, ADC1};
    static const uint8_t lChannelList[ADC_PHASE_COUNT] = {ADC_CHANNEL_15, ADC_CHANNEL_8, ADC_CHANNEL_9};
    uint8_t lPhase;

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
    rcu_periph_clock_enable(RCU_ADC0);
    rcu_periph_clock_enable(RCU_ADC1);
    rcu_periph_clock_enable(RCU_ADC2);
    adc_deinit();
    adc_sync_mode_config(ADC_SYNC_MODE_INDEPENDENT);
    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);

    adcRoutineInit();
    adcCurrentInit();

    /* One software trigger starts the scan; inserted current samples preempt it. */
    adc_software_trigger_enable(ADC1, ADC_ROUTINE_CHANNEL);
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

    gAdcRaw[ADC_CH_IU] = adc_inserted_data_read(ADC2, ADC_INSERTED_CHANNEL_0);
    gAdcRaw[ADC_CH_IV] = adc_inserted_data_read(ADC0, ADC_INSERTED_CHANNEL_0);
    gAdcRaw[ADC_CH_IW] = adc_inserted_data_read(ADC1, ADC_INSERTED_CHANNEL_0);
    if (gAdcCurrentCallback != NULL) {
        gAdcCurrentCallback();
    }
}

/**************************End of file********************************/
