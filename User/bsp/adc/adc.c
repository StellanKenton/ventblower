/************************************************************************************
* @file     : adc.c
* @brief    : Board ADC acquisition driver.
* @details  : ADC0/1 inserted groups sample two of the U/V/W phases together on the
*             TIMER0 CH3 trigger; the current callback picks the phase to skip.
*             ADC1 routine scan with DMA1 circular mode samples VBUS and NTC.
* @author   : Stellan
* @date     :
* @version  :
* @copyright: Copyright (c) 2026 ResMed. All rights reserved.
***********************************************************************************/
#include "adc.h"

#include <stddef.h>

#include "gd32f4xx.h"
#include "gd32f4xx_adc.h"
#include "gd32f4xx_dma.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"

volatile uint16_t gAdcRaw[ADC_CH_COUNT];

static const uint32_t gAdcCurrentList[ADC_SAMPLED_PHASE_COUNT] = {ADC0, ADC1};
/* Indexed by eAdcChannel phase slots. PC5/PB0/PB1 are only routed to ADC0 and ADC1. */
static const uint8_t gAdcPhaseChannel[ADC_PHASE_COUNT] = {ADC_CHANNEL_15, ADC_CHANNEL_8, ADC_CHANNEL_9};
/* Sampled phases per skipped phase, in gAdcCurrentList order. */
static const eAdcChannel gAdcPhasePair[ADC_PHASE_COUNT][ADC_SAMPLED_PHASE_COUNT] = {
    {ADC_CH_IV, ADC_CH_IW},
    {ADC_CH_IU, ADC_CH_IW},
    {ADC_CH_IU, ADC_CH_IV}
};

static pfAdcCurrentCallback gAdcCurrentCallback = NULL;
/* Phase left out of the armed conversion. Only changes in the ISR once running. */
static eAdcChannel gAdcSkipPhase = ADC_CH_IW;

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

/** Arm the two phases sampled on the next trigger. */
static void adcCurrentPairSelect(eAdcChannel skipPhase)
{
    uint8_t lIndex;

    for (lIndex = 0U; lIndex < ADC_SAMPLED_PHASE_COUNT; ++lIndex) {
        adc_inserted_channel_config(gAdcCurrentList[lIndex], 0U,
                                    gAdcPhaseChannel[gAdcPhasePair[skipPhase][lIndex]], ADC_SAMPLETIME_15);
    }
    gAdcSkipPhase = skipPhase;
}

static void adcCurrentInit(void)
{
    uint8_t lIndex;

    /* Same trigger and sample time on both ADCs keeps the two conversions aligned. */
    for (lIndex = 0U; lIndex < ADC_SAMPLED_PHASE_COUNT; ++lIndex) {
        uint32_t lAdc = gAdcCurrentList[lIndex];

        adc_resolution_config(lAdc, ADC_RESOLUTION_12B);
        adc_data_alignment_config(lAdc, ADC_DATAALIGN_RIGHT);
        adc_channel_length_config(lAdc, ADC_INSERTED_CHANNEL, 1U);
        adc_external_trigger_source_config(lAdc, ADC_INSERTED_CHANNEL, ADC_EXTTRIG_INSERTED_T0_CH3);
        adc_external_trigger_config(lAdc, ADC_INSERTED_CHANNEL, EXTERNAL_TRIGGER_RISING);
        adc_enable(lAdc);
        adc_calibration_enable(lAdc);
    }
    adcCurrentPairSelect(gAdcSkipPhase);

    /* ADC1 may start late when it preempts its routine scan, so it finishes last. */
    adc_interrupt_flag_clear(ADC1, ADC_INT_FLAG_EOIC);
    adc_interrupt_enable(ADC1, ADC_INT_EOIC);
    NVIC_SetPriority(ADC_IRQn, ADC_CURRENT_IRQ_PRIORITY);
    NVIC_EnableIRQ(ADC_IRQn);
}

void adcInit(void)
{
    adcGpioInit();
    rcu_periph_clock_enable(RCU_ADC0);
    rcu_periph_clock_enable(RCU_ADC1);
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

/** ADC1 EOIC fires once per PWM period, after ADC0 has finished too. */
void ADC_IRQHandler(void)
{
    const eAdcChannel *lPair = gAdcPhasePair[gAdcSkipPhase];

    if (adc_interrupt_flag_get(ADC1, ADC_INT_FLAG_EOIC) == RESET) {
        return;
    }
    adc_interrupt_flag_clear(ADC1, ADC_INT_FLAG_EOIC);

    gAdcRaw[lPair[0]] = adc_inserted_data_read(ADC0, ADC_INSERTED_CHANNEL_0);
    gAdcRaw[lPair[1]] = adc_inserted_data_read(ADC1, ADC_INSERTED_CHANNEL_0);
    if (gAdcCurrentCallback != NULL) {
        eAdcChannel lNextSkip = gAdcCurrentCallback(gAdcSkipPhase);

        if (((uint8_t)lNextSkip < ADC_PHASE_COUNT) && (lNextSkip != gAdcSkipPhase)) {
            adcCurrentPairSelect(lNextSkip);
        }
    }
}

/**************************End of file********************************/
