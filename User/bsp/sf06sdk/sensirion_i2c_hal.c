/************************************************************************************
* @file     : sensirion_i2c_hal.c
* @brief    : SFM3119 I2C platform binding.
* @details  : Uses I2C0 on PB6/PB7 for air and software I2C on PA12/PA11 for O2.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "sensirion_i2c_hal.h"

#include "gd32f4xx_dma.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_i2c.h"
#include "gd32f4xx_rcu.h"
#include "system_gd32f4xx.h"

static uint8_t gSensirionI2cBus = SENSIRION_I2C_HAL_BUS_AIR;
static uint8_t gSensirionI2cAsyncActive = 0U;
static uint32_t gSensirionI2cAsyncStartCycles = 0U;

static void sensirionI2cHardwareRecover(void) {
    uint8_t lIndex;

    gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_6 | GPIO_PIN_7);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_6 | GPIO_PIN_7);
    gpio_bit_set(GPIOB, GPIO_PIN_6 | GPIO_PIN_7);
    sensirion_i2c_hal_sleep_usec(10U);
    for (lIndex = 0U; lIndex < 9U; lIndex++) {
        gpio_bit_reset(GPIOB, GPIO_PIN_6);
        sensirion_i2c_hal_sleep_usec(10U);
        gpio_bit_set(GPIOB, GPIO_PIN_6);
        sensirion_i2c_hal_sleep_usec(10U);
    }
    gpio_bit_reset(GPIOB, GPIO_PIN_7);
    sensirion_i2c_hal_sleep_usec(10U);
    gpio_bit_set(GPIOB, GPIO_PIN_6);
    sensirion_i2c_hal_sleep_usec(10U);
    gpio_bit_set(GPIOB, GPIO_PIN_7);
    sensirion_i2c_hal_sleep_usec(10U);
}

static void sensirionI2cSoftDelay(void) {
    sensirion_i2c_hal_sleep_usec(SENSIRION_I2C_HAL_SOFT_HALF_PERIOD_US);
}

static void sensirionI2cSoftSetLine(uint32_t pin, uint8_t high) {
    if (high != 0U) {
        gpio_bit_set(GPIOA, pin);
    } else {
        gpio_bit_reset(GPIOA, pin);
    }
}

static void sensirionI2cSoftStart(void) {
    sensirionI2cSoftSetLine(GPIO_PIN_11, 1U);
    sensirionI2cSoftSetLine(GPIO_PIN_12, 1U);
    sensirionI2cSoftDelay();
    sensirionI2cSoftSetLine(GPIO_PIN_11, 0U);
    sensirionI2cSoftDelay();
    sensirionI2cSoftSetLine(GPIO_PIN_12, 0U);
}

static void sensirionI2cSoftStop(void) {
    sensirionI2cSoftSetLine(GPIO_PIN_11, 0U);
    sensirionI2cSoftDelay();
    sensirionI2cSoftSetLine(GPIO_PIN_12, 1U);
    sensirionI2cSoftDelay();
    sensirionI2cSoftSetLine(GPIO_PIN_11, 1U);
    sensirionI2cSoftDelay();
}

static int8_t sensirionI2cSoftWriteByte(uint8_t value) {
    uint8_t lMask;

    for (lMask = 0x80U; lMask != 0U; lMask >>= 1U) {
        sensirionI2cSoftSetLine(GPIO_PIN_11, ((value & lMask) != 0U) ? 1U : 0U);
        sensirionI2cSoftDelay();
        sensirionI2cSoftSetLine(GPIO_PIN_12, 1U);
        sensirionI2cSoftDelay();
        sensirionI2cSoftSetLine(GPIO_PIN_12, 0U);
    }

    sensirionI2cSoftSetLine(GPIO_PIN_11, 1U);
    sensirionI2cSoftDelay();
    sensirionI2cSoftSetLine(GPIO_PIN_12, 1U);
    sensirionI2cSoftDelay();
    if (gpio_input_bit_get(GPIOA, GPIO_PIN_11) != RESET) {
        sensirionI2cSoftSetLine(GPIO_PIN_12, 0U);
        return SENSIRION_I2C_HAL_ERROR_NACK;
    }
    sensirionI2cSoftSetLine(GPIO_PIN_12, 0U);
    return SENSIRION_I2C_HAL_STATUS_OK;
}

static uint8_t sensirionI2cSoftReadByte(uint8_t acknowledge) {
    uint8_t lIndex;
    uint8_t lValue = 0U;

    sensirionI2cSoftSetLine(GPIO_PIN_11, 1U);
    for (lIndex = 0U; lIndex < 8U; lIndex++) {
        lValue <<= 1U;
        sensirionI2cSoftSetLine(GPIO_PIN_12, 1U);
        sensirionI2cSoftDelay();
        if (gpio_input_bit_get(GPIOA, GPIO_PIN_11) != RESET) {
            lValue |= 1U;
        }
        sensirionI2cSoftSetLine(GPIO_PIN_12, 0U);
        sensirionI2cSoftDelay();
    }

    sensirionI2cSoftSetLine(GPIO_PIN_11, (acknowledge != 0U) ? 0U : 1U);
    sensirionI2cSoftDelay();
    sensirionI2cSoftSetLine(GPIO_PIN_12, 1U);
    sensirionI2cSoftDelay();
    sensirionI2cSoftSetLine(GPIO_PIN_12, 0U);
    sensirionI2cSoftSetLine(GPIO_PIN_11, 1U);
    return lValue;
}

static int8_t sensirionI2cSoftWrite(uint8_t address, const uint8_t* data, uint8_t count) {
    uint8_t lIndex;
    int8_t lStatus;

    sensirionI2cSoftStart();
    lStatus = sensirionI2cSoftWriteByte((uint8_t)(address << 1U));
    for (lIndex = 0U; (lStatus == SENSIRION_I2C_HAL_STATUS_OK) && (lIndex < count); lIndex++) {
        lStatus = sensirionI2cSoftWriteByte(data[lIndex]);
    }
    sensirionI2cSoftStop();
    return lStatus;
}

static int8_t sensirionI2cSoftRead(uint8_t address, uint8_t* data, uint8_t count) {
    uint8_t lIndex;
    int8_t lStatus;

    sensirionI2cSoftStart();
    lStatus = sensirionI2cSoftWriteByte((uint8_t)((address << 1U) | 1U));
    for (lIndex = 0U; (lStatus == SENSIRION_I2C_HAL_STATUS_OK) && (lIndex < count); lIndex++) {
        data[lIndex] = sensirionI2cSoftReadByte((lIndex + 1U < count) ? 1U : 0U);
    }
    sensirionI2cSoftStop();
    return lStatus;
}

static int8_t sensirionI2cHardwareWaitSet(i2c_flag_enum flag) {
    uint32_t lStart = DWT->CYCCNT;
    uint32_t lTimeoutCycles = (SystemCoreClock / 1000000U) * SENSIRION_I2C_HAL_TIMEOUT_US;

    while (i2c_flag_get(I2C0, flag) == RESET) {
        if (i2c_flag_get(I2C0, I2C_FLAG_AERR) != RESET) {
            i2c_flag_clear(I2C0, I2C_FLAG_AERR);
            return SENSIRION_I2C_HAL_ERROR_NACK;
        }
        if (i2c_flag_get(I2C0, I2C_FLAG_BERR) != RESET) {
            i2c_flag_clear(I2C0, I2C_FLAG_BERR);
            return SENSIRION_I2C_HAL_ERROR_BUS;
        }
        if ((uint32_t)(DWT->CYCCNT - lStart) >= lTimeoutCycles) {
            return SENSIRION_I2C_HAL_ERROR_TIMEOUT;
        }
    }
    return SENSIRION_I2C_HAL_STATUS_OK;
}

static int8_t sensirionI2cHardwareWaitIdle(void) {
    uint32_t lStart = DWT->CYCCNT;
    uint32_t lTimeoutCycles = (SystemCoreClock / 1000000U) * SENSIRION_I2C_HAL_TIMEOUT_US;

    while (i2c_flag_get(I2C0, I2C_FLAG_I2CBSY) != RESET) {
        if ((uint32_t)(DWT->CYCCNT - lStart) >= lTimeoutCycles) {
            return SENSIRION_I2C_HAL_ERROR_TIMEOUT;
        }
    }
    return SENSIRION_I2C_HAL_STATUS_OK;
}

static int8_t sensirionI2cHardwareStart(uint8_t address, uint32_t direction) {
    int8_t lStatus = sensirionI2cHardwareWaitIdle();

    if (lStatus != SENSIRION_I2C_HAL_STATUS_OK) {
        return (lStatus == SENSIRION_I2C_HAL_ERROR_TIMEOUT) ? SENSIRION_I2C_HAL_ERROR_TIMEOUT_IDLE : lStatus;
    }
    i2c_start_on_bus(I2C0);
    lStatus = sensirionI2cHardwareWaitSet(I2C_FLAG_SBSEND);
    if (lStatus != SENSIRION_I2C_HAL_STATUS_OK) {
        return (lStatus == SENSIRION_I2C_HAL_ERROR_TIMEOUT) ? SENSIRION_I2C_HAL_ERROR_TIMEOUT_START : lStatus;
    }
    i2c_master_addressing(I2C0, (uint32_t)(address << 1U), direction);
    lStatus = sensirionI2cHardwareWaitSet(I2C_FLAG_ADDSEND);
    return (lStatus == SENSIRION_I2C_HAL_ERROR_TIMEOUT) ? SENSIRION_I2C_HAL_ERROR_TIMEOUT_ADDR : lStatus;
}

static int8_t sensirionI2cHardwareWrite(uint8_t address, const uint8_t* data, uint8_t count) {
    uint8_t lIndex;
    int8_t lStatus = sensirionI2cHardwareStart(address, I2C_TRANSMITTER);

    if (lStatus != SENSIRION_I2C_HAL_STATUS_OK) {
        i2c_stop_on_bus(I2C0);
        return lStatus;
    }
    i2c_flag_clear(I2C0, I2C_FLAG_ADDSEND);
    for (lIndex = 0U; lIndex < count; lIndex++) {
        lStatus = sensirionI2cHardwareWaitSet(I2C_FLAG_TBE);
        if (lStatus != SENSIRION_I2C_HAL_STATUS_OK) {
            i2c_stop_on_bus(I2C0);
            return (lStatus == SENSIRION_I2C_HAL_ERROR_TIMEOUT) ? SENSIRION_I2C_HAL_ERROR_TIMEOUT_TX : lStatus;
        }
        i2c_data_transmit(I2C0, data[lIndex]);
    }
    lStatus = sensirionI2cHardwareWaitSet(I2C_FLAG_BTC);
    i2c_stop_on_bus(I2C0);
    return (lStatus == SENSIRION_I2C_HAL_ERROR_TIMEOUT) ? SENSIRION_I2C_HAL_ERROR_TIMEOUT_BTC : lStatus;
}

static int8_t sensirionI2cHardwareRead(uint8_t address, uint8_t* data, uint8_t count) {
    uint8_t lRemaining = count;
    uint8_t* lOutput = data;
    int8_t lStatus;

    i2c_ack_config(I2C0, I2C_ACK_ENABLE);
    lStatus = sensirionI2cHardwareStart(address, I2C_RECEIVER);
    if (lStatus != SENSIRION_I2C_HAL_STATUS_OK) {
        i2c_stop_on_bus(I2C0);
        return lStatus;
    }
    if (count == 1U) {
        i2c_ack_config(I2C0, I2C_ACK_DISABLE);
    }
    i2c_flag_clear(I2C0, I2C_FLAG_ADDSEND);

    while (lRemaining > 0U) {
        if (lRemaining == 3U) {
            lStatus = sensirionI2cHardwareWaitSet(I2C_FLAG_BTC);
            if (lStatus != SENSIRION_I2C_HAL_STATUS_OK) {
                i2c_stop_on_bus(I2C0);
                return (lStatus == SENSIRION_I2C_HAL_ERROR_TIMEOUT) ? SENSIRION_I2C_HAL_ERROR_TIMEOUT_BTC : lStatus;
            }
            i2c_ack_config(I2C0, I2C_ACK_DISABLE);
        } else if (lRemaining == 2U) {
            lStatus = sensirionI2cHardwareWaitSet(I2C_FLAG_BTC);
            if (lStatus != SENSIRION_I2C_HAL_STATUS_OK) {
                i2c_stop_on_bus(I2C0);
                return (lStatus == SENSIRION_I2C_HAL_ERROR_TIMEOUT) ? SENSIRION_I2C_HAL_ERROR_TIMEOUT_BTC : lStatus;
            }
            i2c_stop_on_bus(I2C0);
            *lOutput++ = i2c_data_receive(I2C0);
            *lOutput = i2c_data_receive(I2C0);
            return SENSIRION_I2C_HAL_STATUS_OK;
        } else if (lRemaining == 1U) {
            i2c_stop_on_bus(I2C0);
        }

        lStatus = sensirionI2cHardwareWaitSet(I2C_FLAG_RBNE);
        if (lStatus != SENSIRION_I2C_HAL_STATUS_OK) {
            i2c_stop_on_bus(I2C0);
            return (lStatus == SENSIRION_I2C_HAL_ERROR_TIMEOUT) ? SENSIRION_I2C_HAL_ERROR_TIMEOUT_RX : lStatus;
        }
        *lOutput++ = i2c_data_receive(I2C0);
        lRemaining--;
    }
    return SENSIRION_I2C_HAL_STATUS_OK;
}

static void sensirionI2cHardwareDmaInit(void) {
    dma_single_data_parameter_struct lDmaConfig;

    rcu_periph_clock_enable(RCU_DMA0);
    dma_channel_disable(DMA0, DMA_CH5);
    dma_deinit(DMA0, DMA_CH5);
    dma_single_data_para_struct_init(&lDmaConfig);
    lDmaConfig.periph_addr = (uint32_t)&I2C_DATA(I2C0);
    lDmaConfig.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    lDmaConfig.memory0_addr = 0U;
    lDmaConfig.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    lDmaConfig.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    lDmaConfig.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
    lDmaConfig.direction = DMA_PERIPH_TO_MEMORY;
    lDmaConfig.number = 1U;
    lDmaConfig.priority = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(DMA0, DMA_CH5, &lDmaConfig);
    dma_channel_subperipheral_select(DMA0, DMA_CH5, DMA_SUBPERI1);
}

static void sensirionI2cHardwareAsyncAbort(void) {
    i2c_dma_config(I2C0, I2C_DMA_OFF);
    dma_channel_disable(DMA0, DMA_CH5);
    i2c_stop_on_bus(I2C0);
    i2c_ack_config(I2C0, I2C_ACK_ENABLE);
    i2c_flag_clear(I2C0, I2C_FLAG_AERR);
    i2c_flag_clear(I2C0, I2C_FLAG_BERR);
    dma_flag_clear(DMA0, DMA_CH5, DMA_FLAG_FTF | DMA_FLAG_FEE | DMA_FLAG_SDE | DMA_FLAG_TAE);
    gSensirionI2cAsyncActive = 0U;
    gSensirionI2cAsyncStartCycles = 0U;
}

int16_t sensirion_i2c_hal_select_bus(uint8_t bus_idx) {
    if (bus_idx >= SENSIRION_I2C_HAL_BUS_COUNT) {
        return SENSIRION_I2C_HAL_ERROR_PARAM;
    }
    gSensirionI2cBus = bus_idx;
    return SENSIRION_I2C_HAL_STATUS_OK;
}

void sensirion_i2c_hal_init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_I2C0);

    sensirionI2cHardwareRecover();
    gpio_af_set(GPIOB, GPIO_AF_4, GPIO_PIN_6 | GPIO_PIN_7);
    gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_6 | GPIO_PIN_7);
    gpio_output_options_set(GPIOB, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_6 | GPIO_PIN_7);
    i2c_deinit(I2C0);
    i2c_clock_config(I2C0, SENSIRION_I2C_HAL_HARDWARE_SPEED_HZ, I2C_DTCY_2);
    i2c_mode_addr_config(I2C0, I2C_I2CMODE_ENABLE, I2C_ADDFORMAT_7BITS, 0U);
    i2c_enable(I2C0);
    i2c_ack_config(I2C0, I2C_ACK_ENABLE);

    sensirionI2cHardwareDmaInit();
    gSensirionI2cAsyncActive = 0U;
    gSensirionI2cAsyncStartCycles = 0U;

    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_11 | GPIO_PIN_12);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_11 | GPIO_PIN_12);
    sensirionI2cSoftSetLine(GPIO_PIN_11, 1U);
    sensirionI2cSoftSetLine(GPIO_PIN_12, 1U);
}

void sensirion_i2c_hal_free(void) {
    sensirionI2cHardwareAsyncAbort();
    dma_deinit(DMA0, DMA_CH5);
    i2c_disable(I2C0);
}

int8_t sensirion_i2c_hal_read(uint8_t address, uint8_t* data, uint8_t count) {
    if ((address > 0x7FU) || (data == NULL) || (count == 0U)) {
        return SENSIRION_I2C_HAL_ERROR_PARAM;
    }
    if (gSensirionI2cBus == SENSIRION_I2C_HAL_BUS_AIR) {
        return sensirionI2cHardwareRead(address, data, count);
    }
    return sensirionI2cSoftRead(address, data, count);
}

int8_t sensirion_i2c_hal_read_async_start(uint8_t address, uint8_t* data, uint8_t count) {
    int8_t lStatus;

    if ((gSensirionI2cBus != SENSIRION_I2C_HAL_BUS_AIR) || (address > 0x7FU) ||
        (data == NULL) || (count == 0U) || (gSensirionI2cAsyncActive != 0U)) {
        return SENSIRION_I2C_HAL_ERROR_PARAM;
    }
    dma_channel_disable(DMA0, DMA_CH5);
    dma_flag_clear(DMA0, DMA_CH5, DMA_FLAG_FTF | DMA_FLAG_FEE | DMA_FLAG_SDE | DMA_FLAG_TAE);
    dma_memory_address_config(DMA0, DMA_CH5, DMA_MEMORY_0, (uint32_t)data);
    dma_transfer_number_config(DMA0, DMA_CH5, count);
    i2c_ack_config(I2C0, I2C_ACK_ENABLE);
    i2c_dma_last_transfer_config(I2C0, I2C_DMALST_ON);
    lStatus = sensirionI2cHardwareStart(address, I2C_RECEIVER);
    if (lStatus != SENSIRION_I2C_HAL_STATUS_OK) {
        sensirionI2cHardwareAsyncAbort();
        return lStatus;
    }
    dma_channel_enable(DMA0, DMA_CH5);
    i2c_dma_config(I2C0, I2C_DMA_ON);
    gSensirionI2cAsyncStartCycles = DWT->CYCCNT;
    gSensirionI2cAsyncActive = 1U;
    i2c_flag_clear(I2C0, I2C_FLAG_ADDSEND);
    return SENSIRION_I2C_HAL_STATUS_OK;
}

int8_t sensirion_i2c_hal_read_async_poll(void) {
    uint32_t lTimeoutCycles = (SystemCoreClock / 1000000U) * SENSIRION_I2C_HAL_TIMEOUT_US;

    if (gSensirionI2cAsyncActive == 0U) {
        return SENSIRION_I2C_HAL_ERROR_PARAM;
    }
    if ((i2c_flag_get(I2C0, I2C_FLAG_AERR) != RESET) ||
        (i2c_flag_get(I2C0, I2C_FLAG_BERR) != RESET) ||
        (dma_flag_get(DMA0, DMA_CH5, DMA_FLAG_FEE) != RESET) ||
        (dma_flag_get(DMA0, DMA_CH5, DMA_FLAG_SDE) != RESET) ||
        (dma_flag_get(DMA0, DMA_CH5, DMA_FLAG_TAE) != RESET)) {
        sensirionI2cHardwareAsyncAbort();
        return SENSIRION_I2C_HAL_ERROR_BUS;
    }
    if (dma_flag_get(DMA0, DMA_CH5, DMA_FLAG_FTF) != RESET) {
        sensirionI2cHardwareAsyncAbort();
        return SENSIRION_I2C_HAL_STATUS_OK;
    }
    if ((uint32_t)(DWT->CYCCNT - gSensirionI2cAsyncStartCycles) >= lTimeoutCycles) {
        sensirionI2cHardwareAsyncAbort();
        return SENSIRION_I2C_HAL_ERROR_TIMEOUT;
    }
    return SENSIRION_I2C_HAL_STATUS_PENDING;
}

void sensirion_i2c_hal_read_async_abort(void) {
    if (gSensirionI2cAsyncActive != 0U) {
        sensirionI2cHardwareAsyncAbort();
    }
}

int8_t sensirion_i2c_hal_write(uint8_t address, const uint8_t* data, uint8_t count) {
    if ((address > 0x7FU) || ((data == NULL) && (count > 0U))) {
        return SENSIRION_I2C_HAL_ERROR_PARAM;
    }
    if (gSensirionI2cBus == SENSIRION_I2C_HAL_BUS_AIR) {
        return sensirionI2cHardwareWrite(address, data, count);
    }
    return sensirionI2cSoftWrite(address, data, count);
}

void sensirion_i2c_hal_sleep_usec(uint32_t useconds) {
    uint32_t lChunk;
    uint32_t lCycles;
    uint32_t lStart;
    uint32_t lCyclesPerUs = SystemCoreClock / 1000000U;

    while (useconds > 0U) {
        lChunk = (useconds > 1000000U) ? 1000000U : useconds;
        lCycles = lCyclesPerUs * lChunk;
        lStart = DWT->CYCCNT;
        while ((uint32_t)(DWT->CYCCNT - lStart) < lCycles) {
        }
        useconds -= lChunk;
    }
}

/**************************End of file********************************/
