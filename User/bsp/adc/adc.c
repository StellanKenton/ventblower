/************************************************************************************
* @file     : adc.c
* @brief    : Board ADC continuous acquisition driver.
* @details  : Uses ADC1 scan/continuous mode and DMA1 circular mode.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "adc.h"

#include "gd32f4xx_adc.h"
#include "gd32f4xx_dma.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"

volatile uint16_t adc_value[ADC_CH_COUNT];

static const uint8_t gAdcChannelList[ADC_CH_COUNT] = {
    ADC_CHANNEL_0,
    ADC_CHANNEL_1,
    ADC_CHANNEL_2,
    ADC_CHANNEL_3,
    ADC_CHANNEL_4,
    ADC_CHANNEL_5,
    ADC_CHANNEL_6,
    ADC_CHANNEL_7,
    ADC_CHANNEL_8,
    ADC_CHANNEL_10,
    ADC_CHANNEL_11,
    ADC_CHANNEL_12,
    ADC_CHANNEL_14,
    ADC_CHANNEL_15
};

static void adcGpioInit(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);

    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
                  GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                  GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    gpio_mode_set(GPIOB, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_0);
    gpio_mode_set(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE,
                  GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5);
}

static void adcDmaInit(void)
{
    dma_single_data_parameter_struct lDmaConfig;

    rcu_periph_clock_enable(RCU_DMA1);
    dma_deinit(DMA1, DMA_CH2);
    dma_single_data_para_struct_init(&lDmaConfig);

    lDmaConfig.periph_addr = (uint32_t)&ADC_RDATA(ADC1);
    lDmaConfig.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    lDmaConfig.memory0_addr = (uint32_t)adc_value;
    lDmaConfig.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    lDmaConfig.periph_memory_width = DMA_PERIPH_WIDTH_16BIT;
    lDmaConfig.circular_mode = DMA_CIRCULAR_MODE_ENABLE;
    lDmaConfig.direction = DMA_PERIPH_TO_MEMORY;
    lDmaConfig.number = ADC_CH_COUNT;
    lDmaConfig.priority = DMA_PRIORITY_HIGH;

    dma_single_data_mode_init(DMA1, DMA_CH2, &lDmaConfig);
    /* ADC1 is mapped to DMA1 channel 2, subperipheral 1. */
    dma_channel_subperipheral_select(DMA1, DMA_CH2, DMA_SUBPERI1);
    dma_channel_enable(DMA1, DMA_CH2);
}

static void adcPeripheralInit(void)
{
    uint8_t lRank;

    rcu_periph_clock_enable(RCU_ADC1);
    adc_deinit();
    adc_sync_mode_config(ADC_SYNC_MODE_INDEPENDENT);
    adc_clock_config(ADC_ADCCK_PCLK2_DIV8);
    adc_resolution_config(ADC1, ADC_RESOLUTION_12B);
    adc_data_alignment_config(ADC1, ADC_DATAALIGN_RIGHT);
    adc_special_function_config(ADC1, ADC_SCAN_MODE, ENABLE);
    adc_special_function_config(ADC1, ADC_CONTINUOUS_MODE, ENABLE);
    adc_channel_length_config(ADC1, ADC_ROUTINE_CHANNEL, ADC_CH_COUNT);

    for (lRank = 0U; lRank < ADC_CH_COUNT; ++lRank) {
        adc_routine_channel_config(ADC1, lRank, gAdcChannelList[lRank], ADC_SAMPLETIME_144);
    }

    adc_external_trigger_config(ADC1, ADC_ROUTINE_CHANNEL, EXTERNAL_TRIGGER_DISABLE);
    adc_dma_request_after_last_enable(ADC1);
    adc_dma_mode_enable(ADC1);
    adc_enable(ADC1);
    adc_calibration_enable(ADC1);
}

void adcInit(void)
{
    adcGpioInit();
    adcDmaInit();
    adcPeripheralInit();

    /* One software trigger starts uninterrupted scan sequences. */
    adc_software_trigger_enable(ADC1, ADC_ROUTINE_CHANNEL);
}

/**************************End of file********************************/
