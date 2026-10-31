/************************************************************************************
* @file     : uart.c
* @brief    : MCM UART transport on USART0 PA9/PA10.
* @details  : Task-owned TX, interrupt-owned RX producer; no RTOS calls in ISR.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "uart.h"
#include "gd32f4xx.h"
#include "gd32f4xx_usart.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"
#include <string.h>
static uint8_t gUartRx[UART_MCM_RX_SIZE];
static uint8_t gUartTx[UART_MCM_TX_SIZE];
static volatile uint32_t gUartRxHead;
static volatile uint32_t gUartRxTail;
static volatile uint16_t gUartTxIndex;
static volatile uint16_t gUartTxLength;
static volatile uint32_t gUartRxDropped;

/** Initialize 8N1 transport before starting the protocol task. */
void uartInit(void) {
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_USART0);
    gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_9 | GPIO_PIN_10);
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9 | GPIO_PIN_10);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9 | GPIO_PIN_10);
    usart_deinit(USART0);
    usart_baudrate_set(USART0, UART_MCM_BAUDRATE);
    usart_word_length_set(USART0, USART_WL_8BIT);
    usart_stop_bit_set(USART0, USART_STB_1BIT);
    usart_parity_config(USART0, USART_PM_NONE);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_enable(USART0);
    NVIC_SetPriority(USART0_IRQn, 6U);
    NVIC_EnableIRQ(USART0_IRQn);
    usart_interrupt_enable(USART0, USART_INT_RBNE);
}

/** Report whether the previous frame has reached the wire. Task context. */
bool uartIsTxBusy(uint8_t instance) {
    return instance != 0U || gUartTxIndex < gUartTxLength || usart_flag_get(USART0, USART_FLAG_TC) == RESET;
}

/** Copy a complete frame and start bounded interrupt-driven transmission. */
int8_t uartSendData(uint8_t instance, const uint8_t *data, uint16_t length) {
    if (instance != 0U || data == NULL || length == 0U || length > sizeof(gUartTx)) { return UART_ERROR_PARAM; }
    if (uartIsTxBusy(instance)) { return UART_ERROR_BUSY; }
    memcpy(gUartTx, data, length);
    gUartTxIndex = 0U;
    gUartTxLength = length;
    __DMB();
    usart_interrupt_enable(USART0, USART_INT_TBE);
    return UART_STATUS_OK;
}

/** Return the number of bytes available to the single protocol consumer. */
uint16_t uartGetRxDataCount(uint8_t instance) {
    return instance == 0U ? (uint16_t)(gUartRxHead - gUartRxTail) : 0U;
}

/** Consume exactly length bytes; never partially consume on an invalid request. */
int8_t uartGetRxData(uint8_t instance, uint8_t *data, uint16_t length) {
    if (data == NULL || instance != 0U || length > uartGetRxDataCount(instance)) { return UART_ERROR_PARAM; }
    for (uint16_t lIndex = 0; lIndex < length; ++lIndex) {
        data[lIndex] = gUartRx[gUartRxTail % UART_MCM_RX_SIZE];
        __DMB();
        ++gUartRxTail;
    }
    return UART_STATUS_OK;
}

/** Service at most one received and one transmitted byte per interrupt. */
void USART0_IRQHandler(void) {
    uint32_t lStatus = USART_STAT0(USART0);
    if ((lStatus & (USART_STAT0_RBNE | USART_STAT0_ORERR | USART_STAT0_FERR | USART_STAT0_NERR)) != 0U) {
        uint8_t lByte = (uint8_t)usart_data_receive(USART0);
        if ((lStatus & (USART_STAT0_ORERR | USART_STAT0_FERR | USART_STAT0_NERR)) != 0U) {
            ++gUartRxDropped;
        } else if (gUartRxHead - gUartRxTail < UART_MCM_RX_SIZE) {
            gUartRx[gUartRxHead % UART_MCM_RX_SIZE] = lByte;
            __DMB();
            ++gUartRxHead;
        } else {
            ++gUartRxDropped;
        }
    }
    if (usart_interrupt_flag_get(USART0, USART_INT_FLAG_TBE) == SET) {
        if (gUartTxIndex < gUartTxLength) { usart_data_transmit(USART0, gUartTx[gUartTxIndex++]); }
        if (gUartTxIndex == gUartTxLength) { usart_interrupt_disable(USART0, USART_INT_TBE); }
    }
}
/**************************End of file********************************/
