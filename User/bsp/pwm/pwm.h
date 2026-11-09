/************************************************************************************
* @file     : pwm.h
* @brief    : Three-phase complementary PWM and blower 28 V power stage enable.
* @details  : TIMER0 CH0/1/2 + CH0N/1N/2N on PE9/PE11/PE13 + PE8/PE10/PE12,
*             center-aligned with dead time; CH3 triggers phase current sampling.
*             PE15 switches the blower 28 V supply.
* @author   : Stellan
* @date     :
* @version  :
* @copyright: Copyright (c) 2026 ResMed. All rights reserved.
***********************************************************************************/
#ifndef USER_BSP_PWM_H
#define USER_BSP_PWM_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TIMER0 runs at 2 x PCLK2 = 240 MHz. */
#define PWM_TIMER_CLOCK_HZ      240000000U
#define PWM_FREQ_HZ             20000U
/* Center-aligned: one PWM period counts up to PWM_PERIOD and back down. */
#define PWM_PERIOD              (PWM_TIMER_CLOCK_HZ / (2U * PWM_FREQ_HZ))
/* DTCFG raw value: 68 ticks = 283 ns at 240 MHz (direct range is 0-127). */
#define PWM_DEAD_TIME           68U
/* PE8-PE13: CH0N, CH0, CH1N, CH1, CH2N, CH2. */
#define PWM_GATE_PINS           (GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13)
/* CH3 rises at the counter peak, where all low-side switches conduct. */
#define PWM_SAMPLE_POINT        (PWM_PERIOD - 1U)

/**
 * @brief Configure TIMER0, start the counter with outputs held off, keep 28 V disabled.
 * @note The counter runs immediately so ADC current offsets can be calibrated
 * before outputs are enabled.
 */
void pwmInit(void);

/**
 * @brief Enable or disable all six gate outputs through the TIMER0 primary output.
 * @param enable true to drive the bridge, false to force all gates low.
 * @note Callable from task or ISR context.
 */
void pwmOutputSet(bool enable);

/**
 * @brief Load phase compare values for the next PWM update.
 * @param u Phase U compare value, 0 to PWM_PERIOD.
 * @param v Phase V compare value, 0 to PWM_PERIOD.
 * @param w Phase W compare value, 0 to PWM_PERIOD.
 * @note Values above PWM_PERIOD are clamped. Intended for the current loop ISR.
 */
void pwmDutySet(uint16_t u, uint16_t v, uint16_t w);

/**
 * @brief Switch the blower 28 V supply on PE15.
 * @param enable true to power the bridge.
 */
void pwmPowerSet(bool enable);

#ifdef __cplusplus
}
#endif

#endif /* USER_BSP_PWM_H */
/**************************End of file********************************/
