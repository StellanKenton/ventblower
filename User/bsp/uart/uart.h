/************************************************************************************
* @file     : uart.h
* @brief    : MCM UART transport on USART0 PA9/PA10.
* @details  : Task-owned TX, interrupt-owned RX producer; no RTOS calls in ISR.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef USER_BSP_UART_H
#define USER_BSP_UART_H
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
#define UART_MCM_BAUDRATE 115200U
#define UART_MCM_RX_SIZE 2048U
#define UART_MCM_TX_SIZE 256U
#define UART_STATUS_OK 1
#define UART_ERROR_PARAM (-1)
#define UART_ERROR_BUSY (-2)
void uartInit(void);
bool uartIsTxBusy(uint8_t instance);
int8_t uartSendData(uint8_t instance, const uint8_t *data, uint16_t length);
uint16_t uartGetRxDataCount(uint8_t instance);
int8_t uartGetRxData(uint8_t instance, uint8_t *data, uint16_t length);
#ifdef __cplusplus
}
#endif
#endif
/**************************End of file********************************/
