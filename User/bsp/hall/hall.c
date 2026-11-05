/************************************************************************************
* @file     : hall.c
* @brief    : Three-phase Hall sensor interface on TIMER3.
* @details  : ISR owns the period and stall state; readers only load 16-bit values.
* @author   : Stellan
* @date     :
* @version  :
* @copyright: Copyright (c) 2026 ResMed. All rights reserved.
***********************************************************************************/
#include "hall.h"

#include <stdbool.h>

#include "gd32f4xx.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"
#include "gd32f4xx_timer.h"

static volatile uint16_t gHallPeriod;
static volatile bool gHallStalled = true;

void hallInit(void) {
    timer_parameter_struct lTimerConfig;
    timer_ic_parameter_struct lCaptureConfig;

    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_TIMER3);
    gpio_af_set(GPIOD, GPIO_AF_2, HALL_PINS);
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, HALL_PINS);

    timer_deinit(TIMER3);
    timer_struct_para_init(&lTimerConfig);
    lTimerConfig.prescaler = HALL_PRESCALER;
    lTimerConfig.alignedmode = TIMER_COUNTER_EDGE;
    lTimerConfig.counterdirection = TIMER_COUNTER_UP;
    lTimerConfig.clockdivision = TIMER_CKDIV_DIV1;
    lTimerConfig.period = 0xFFFFU;
    timer_init(TIMER3, &lTimerConfig);

    /* CH0 captures on the XOR edge detector, which also restarts the counter. */
    timer_channel_input_struct_para_init(&lCaptureConfig);
    lCaptureConfig.icpolarity = TIMER_IC_POLARITY_RISING;
    lCaptureConfig.icselection = TIMER_IC_SELECTION_ITS;
    lCaptureConfig.icprescaler = TIMER_IC_PSC_DIV1;
    lCaptureConfig.icfilter = HALL_INPUT_FILTER;
    timer_input_capture_config(TIMER3, TIMER_CH_0, &lCaptureConfig);
    timer_hall_mode_config(TIMER3, TIMER_HALLINTERFACE_ENABLE);
    timer_input_trigger_source_select(TIMER3, TIMER_SMCFG_TRGSEL_CI0F_ED);
    timer_slave_mode_select(TIMER3, TIMER_SLAVE_MODE_RESTART);
    /* Only a real overflow raises UP, so it means no edge for a full counter span. */
    timer_update_source_config(TIMER3, TIMER_UPDATE_SRC_REGULAR);

    timer_interrupt_flag_clear(TIMER3, TIMER_INT_FLAG_CH0 | TIMER_INT_FLAG_UP);
    timer_interrupt_enable(TIMER3, TIMER_INT_CH0 | TIMER_INT_UP);
    NVIC_SetPriority(TIMER3_IRQn, HALL_IRQ_PRIORITY);
    NVIC_EnableIRQ(TIMER3_IRQn);
    timer_enable(TIMER3);
}

uint8_t hallStateGet(void) {
    return (uint8_t)((GPIO_ISTAT(GPIOD) >> HALL_PIN_SHIFT) & HALL_STATE_MASK);
}

uint16_t hallPeriodGet(void) {
    return gHallPeriod;
}

uint16_t hallElapsedGet(void) {
    return (uint16_t)timer_counter_read(TIMER3);
}

/** The first edge after a stall only re-arms timing; its capture spans the overflow. */
void TIMER3_IRQHandler(void) {
    if (timer_interrupt_flag_get(TIMER3, TIMER_INT_FLAG_CH0) == SET) {
        timer_interrupt_flag_clear(TIMER3, TIMER_INT_FLAG_CH0);
        gHallPeriod = gHallStalled ? 0U : (uint16_t)timer_channel_capture_value_register_read(TIMER3, TIMER_CH_0);
        gHallStalled = false;
    }
    if (timer_interrupt_flag_get(TIMER3, TIMER_INT_FLAG_UP) == SET) {
        timer_interrupt_flag_clear(TIMER3, TIMER_INT_FLAG_UP);
        gHallPeriod = 0U;
        gHallStalled = true;
    }
}

/**************************End of file********************************/
