/************************************************************************************
* @file     : pwm.c
* @brief    : Three-phase complementary PWM and blower 28 V power stage enable.
* @details  : TIMER0 center-aligned PWM, active-high gates, idle low; CH3 is the
*             ADC inserted trigger. Counter halts with the core under debug.
* @author   : Stellan
* @date     :
* @version  :
* @copyright: Copyright (c) 2026 ResMed. All rights reserved.
***********************************************************************************/
#include "pwm.h"

#include "gd32f4xx_dbg.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"
#include "gd32f4xx_timer.h"

/** Hold 28 V off and pull gate pins low before TIMER0 owns them. */
static void pwmGpioInit(void) {
    rcu_periph_clock_enable(RCU_GPIOE);

    gpio_bit_reset(GPIOE, GPIO_PIN_15);
    gpio_mode_set(GPIOE, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_15);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, GPIO_PIN_15);

    gpio_af_set(GPIOE, GPIO_AF_1, PWM_GATE_PINS);
    gpio_mode_set(GPIOE, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, PWM_GATE_PINS);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, PWM_GATE_PINS);
}

/** Configure one phase pair, or CH3 as the ADC trigger with no pin. */
static void pwmChannelInit(uint16_t channel, uint16_t mode, uint16_t pulse) {
    timer_oc_parameter_struct lOutputConfig;

    timer_channel_output_struct_para_init(&lOutputConfig);
    lOutputConfig.outputstate = TIMER_CCX_ENABLE;
    lOutputConfig.outputnstate = (channel == TIMER_CH_3) ? TIMER_CCXN_DISABLE : TIMER_CCXN_ENABLE;
    lOutputConfig.ocpolarity = TIMER_OC_POLARITY_HIGH;
    lOutputConfig.ocnpolarity = TIMER_OCN_POLARITY_HIGH;
    lOutputConfig.ocidlestate = TIMER_OC_IDLE_STATE_LOW;
    lOutputConfig.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
    timer_channel_output_config(TIMER0, channel, &lOutputConfig);
    timer_channel_output_mode_config(TIMER0, channel, mode);
    timer_channel_output_pulse_value_config(TIMER0, channel, pulse);
    timer_channel_output_shadow_config(TIMER0, channel, TIMER_OC_SHADOW_ENABLE);
}

void pwmInit(void) {
    timer_parameter_struct lTimerConfig;
    timer_break_parameter_struct lBreakConfig;

    pwmGpioInit();
    rcu_periph_clock_enable(RCU_TIMER0);
    /* Stop PWM and force outputs idle while the core is halted by the debugger. */
    dbg_periph_enable(DBG_TIMER0_HOLD);
    timer_deinit(TIMER0);

    timer_struct_para_init(&lTimerConfig);
    lTimerConfig.prescaler = 0U;
    lTimerConfig.alignedmode = TIMER_COUNTER_CENTER_UP;
    lTimerConfig.counterdirection = TIMER_COUNTER_UP;
    lTimerConfig.clockdivision = TIMER_CKDIV_DIV1;
    lTimerConfig.period = PWM_PERIOD;
    lTimerConfig.repetitioncounter = 0U;
    timer_init(TIMER0, &lTimerConfig);

    pwmChannelInit(TIMER_CH_0, TIMER_OC_MODE_PWM0, PWM_PERIOD / 2U);
    pwmChannelInit(TIMER_CH_1, TIMER_OC_MODE_PWM0, PWM_PERIOD / 2U);
    pwmChannelInit(TIMER_CH_2, TIMER_OC_MODE_PWM0, PWM_PERIOD / 2U);
    pwmChannelInit(TIMER_CH_3, TIMER_OC_MODE_PWM1, PWM_SAMPLE_POINT);

    /* Off-state selection keeps gates driven to idle low whenever POEN is cleared. */
    timer_break_struct_para_init(&lBreakConfig);
    lBreakConfig.runoffstate = TIMER_ROS_STATE_ENABLE;
    lBreakConfig.ideloffstate = TIMER_IOS_STATE_ENABLE;
    lBreakConfig.deadtime = PWM_DEAD_TIME;
    lBreakConfig.breakpolarity = TIMER_BREAK_POLARITY_HIGH;
    lBreakConfig.outputautostate = TIMER_OUTAUTO_DISABLE;
    lBreakConfig.protectmode = TIMER_CCHP_PROT_OFF;
    lBreakConfig.breakstate = TIMER_BREAK_DISABLE;
    timer_break_config(TIMER0, &lBreakConfig);

    timer_primary_output_config(TIMER0, DISABLE);
    timer_auto_reload_shadow_enable(TIMER0);
    timer_enable(TIMER0);
}

void pwmOutputSet(bool enable) {
    timer_primary_output_config(TIMER0, enable ? ENABLE : DISABLE);
}

void pwmDutySet(uint16_t u, uint16_t v, uint16_t w) {
    TIMER_CH0CV(TIMER0) = (u > PWM_PERIOD) ? PWM_PERIOD : u;
    TIMER_CH1CV(TIMER0) = (v > PWM_PERIOD) ? PWM_PERIOD : v;
    TIMER_CH2CV(TIMER0) = (w > PWM_PERIOD) ? PWM_PERIOD : w;
}

void pwmPowerSet(bool enable) {
    gpio_bit_write(GPIOE, GPIO_PIN_15, enable ? SET : RESET);
}

/**************************End of file********************************/
