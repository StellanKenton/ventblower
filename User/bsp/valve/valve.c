/************************************************************************************
* @file     : valve.c
* @brief    : Board zeroing valve GPIO driver.
* @details  : Configures four valve outputs and their corresponding state inputs.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "valve.h"

#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"

static const uint32_t gValveControlPorts[VALVE_COUNT] = {
    GPIOD,
    GPIOD,
    GPIOB,
    GPIOD
};

static const uint32_t gValveControlPins[VALVE_COUNT] = {
    GPIO_PIN_0,
    GPIO_PIN_1,
    GPIO_PIN_9,
    GPIO_PIN_3
};

static const uint32_t gValveStatePorts[VALVE_COUNT] = {
    GPIOB,
    GPIOB,
    GPIOE,
    GPIOB
};

static const uint32_t gValveStatePins[VALVE_COUNT] = {
    GPIO_PIN_10,
    GPIO_PIN_11,
    GPIO_PIN_14,
    GPIO_PIN_13
};

void valveInit(void)
{
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);

    /* Apply a known inactive level before enabling the output drivers. */
    gpio_bit_reset(GPIOB, GPIO_PIN_9);
    gpio_bit_reset(GPIOD, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3);

    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_9);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
    gpio_mode_set(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3);

    gpio_mode_set(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_NONE,
                  GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_13);
    gpio_mode_set(GPIOE, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO_PIN_14);
}

int8_t valveControlSet(eValveIndex valveIndex, eValveLevel level)
{
    if ((uint32_t)valveIndex >= (uint32_t)VALVE_COUNT) {
        return VALVE_ERROR_INVALID_INDEX;
    }
    if ((level != VALVE_LEVEL_LOW) && (level != VALVE_LEVEL_HIGH)) {
        return VALVE_ERROR_INVALID_LEVEL;
    }

    gpio_bit_write(gValveControlPorts[valveIndex], gValveControlPins[valveIndex],
                   (level == VALVE_LEVEL_HIGH) ? SET : RESET);
    return VALVE_STATUS_OK;
}

eValveLevel valveStateGet(eValveIndex valveIndex)
{
    if ((uint32_t)valveIndex >= (uint32_t)VALVE_COUNT) {
        return VALVE_LEVEL_INVALID;
    }
    return (gpio_input_bit_get(gValveStatePorts[valveIndex], gValveStatePins[valveIndex]) == SET) ?
           VALVE_LEVEL_HIGH : VALVE_LEVEL_LOW;
}

/**************************End of file********************************/
