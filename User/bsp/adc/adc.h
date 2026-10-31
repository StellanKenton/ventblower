/************************************************************************************
* @file     : adc.h
* @brief    : Board ADC continuous acquisition interface.
* @details  : Defines ADC DMA buffer indexes and the initialization entry point.
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

typedef enum eAdcChannelIndex {
    ADC_IDX_SF_VALVE_CURRENT = 0,
    ADC_IDX_SFM_FLOW,
    ADC_IDX_HW_VERSION,
    ADC_IDX_PCM_3V3,
    ADC_IDX_OUT_TEMP,
    ADC_IDX_24V,
    ADC_IDX_PEEP_VALVE_CURRENT,
    ADC_IDX_INSP_PRS,
    ADC_IDX_AVDD5V,
    ADC_IDX_EXP_PRS,    
    ADC_IDX_PEEP_PRS,   
    ADC_IDX_MDIFF_PRS,
    ADC_IDX_O2_VALVE_CURRENT,
    ADC_IDX_VDD_26V,
    ADC_CH_COUNT
} eAdcChannelIndex;
// 硬件上以及旧ad中，peep和exp反

extern volatile uint16_t adc_value[ADC_CH_COUNT];

void adcInit(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_BSP_ADC_H */
/**************************End of file********************************/
