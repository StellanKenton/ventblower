/************************************************************************************
* @file     : hall.h
* @brief    : Three-phase Hall sensor interface on TIMER3.
* @details  : PD12/PD13/PD14 = Hall A/B/C = TIMER3 CH0/CH1/CH2. Hall mode XORs
*             the three inputs; every edge captures the edge interval and
*             restarts the counter. A counter overflow marks the rotor stalled.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef USER_BSP_HALL_H
#define USER_BSP_HALL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TIMER3 runs at 2 x PCLK1 = 120 MHz. */
#define HALL_TIMER_CLOCK_HZ     120000000U
/* 1 us capture tick; 16-bit counter overflows after 65.5 ms (stall). */
#define HALL_TICK_HZ            1000000U
#define HALL_PRESCALER          (HALL_TIMER_CLOCK_HZ / HALL_TICK_HZ - 1U)
/* Input filter fDTS/32, N=8: about 2.1 us glitch rejection. */
#define HALL_INPUT_FILTER       0x0FU
#define HALL_IRQ_PRIORITY       2U
#define HALL_PINS               (GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14)
#define HALL_PIN_SHIFT          12U
#define HALL_STATE_MASK         0x07U

/**
 * @brief Configure PD12/PD13/PD14 and TIMER3 hall capture, then start it.
 */
void hallInit(void);

/**
 * @brief Read the instantaneous Hall state.
 * @return bit0 = A, bit1 = B, bit2 = C; 0 or 7 means a sensor fault.
 * @note Callable from task or ISR context.
 */
uint8_t hallStateGet(void);

/**
 * @brief Get the interval between the last two Hall edges.
 * @return Interval in HALL_TICK_HZ ticks, or 0 when stalled or not yet measured.
 */
uint16_t hallPeriodGet(void);

/**
 * @brief Get the time since the last Hall edge for angle interpolation.
 * @return Elapsed HALL_TICK_HZ ticks; meaningful only while hallPeriodGet() != 0.
 */
uint16_t hallElapsedGet(void);

/** @brief TIMER3 interrupt entry point. */
void TIMER3_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_BSP_HALL_H */
/**************************End of file********************************/
