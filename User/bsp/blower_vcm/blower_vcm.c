/************************************************************************************
* @file     : blower_vcm.c
* @brief    : VCM blower UART communication BSP.
* @details  : Implements circular DMA reception, stream parsing, and reliable control TX.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#include "blower_vcm.h"

#include <stddef.h>

#include "gd32f4xx.h"
#include "gd32f4xx_dma.h"
#include "gd32f4xx_gpio.h"
#include "gd32f4xx_rcu.h"
#include "gd32f4xx_usart.h"
#include "rtos.h"

static uint8_t gBlowerVcmRxDmaBuffer[BLOWER_VCM_RX_DMA_SIZE];
static uint8_t gBlowerVcmRxFifo[BLOWER_VCM_RX_FIFO_SIZE];
static uint8_t gBlowerVcmTxBuffer[BLOWER_VCM_TX_DMA_SIZE];
static uint8_t gBlowerVcmParseBuffer[BLOWER_VCM_MAX_FRAME_LENGTH];
static volatile uint16_t gBlowerVcmRxWrite = 0U;
static volatile uint16_t gBlowerVcmRxRead = 0U;
static volatile uint16_t gBlowerVcmDmaReadPos = 0U;
static volatile bool gBlowerVcmTxBusy = false;
static volatile bool gBlowerVcmInitialized = false;
static volatile bool gBlowerVcmRxOverflow = false;
static volatile bool gBlowerVcmDmaCollecting = false;
static bool gBlowerVcmLastControlValid = false;
static eBlowerVcmControlMode gBlowerVcmLastMode = BLOWER_CTRL_INIT;
static uint16_t gBlowerVcmLastTargetValue = 0U;
static uint8_t gBlowerVcmLastSaturation = 0U;
static uint32_t gBlowerVcmLastTxAtMs = 0U;
static uint8_t gBlowerVcmParseState = BLOWER_VCM_PARSE_WAIT_HEADER;
static uint8_t gBlowerVcmParseIndex = 0U;
static uint8_t gBlowerVcmParseDataLength = 0U;
static uint32_t gBlowerVcmProcessNowMs = 0U;
static stBlowerVcmFeedback gBlowerVcmFeedback;
static stBlowerVcmStats gBlowerVcmStats;

static uint16_t blowerVcmCrc16(const uint8_t *data, uint8_t length)
{
    uint16_t lCrc = 0xFFFFU;
    uint8_t lByteIndex;
    uint8_t lBitIndex;

    for (lByteIndex = 0U; lByteIndex < length; lByteIndex++) {
        lCrc ^= data[lByteIndex];
        for (lBitIndex = 0U; lBitIndex < 8U; lBitIndex++) {
            if ((lCrc & 1U) != 0U) {
                lCrc = (uint16_t)((lCrc >> 1U) ^ 0xA001U);
            } else {
                lCrc >>= 1U;
            }
        }
    }
    return lCrc;
}

static void blowerVcmMemoryClear(uint8_t *data, uint16_t length)
{
    uint16_t lIndex;

    for (lIndex = 0U; lIndex < length; lIndex++) {
        data[lIndex] = 0U;
    }
}

static void blowerVcmStatsClear(void)
{
    gBlowerVcmStats = (stBlowerVcmStats){0};
}

static void blowerVcmParserReset(uint8_t candidate)
{
    if (candidate == BLOWER_VCM_FRAME_HEADER) {
        gBlowerVcmParseBuffer[0] = candidate;
        gBlowerVcmParseIndex = 1U;
        gBlowerVcmParseState = BLOWER_VCM_PARSE_COMMAND;
    } else {
        gBlowerVcmParseIndex = 0U;
        gBlowerVcmParseState = BLOWER_VCM_PARSE_WAIT_HEADER;
    }
    gBlowerVcmParseDataLength = 0U;
}

static void blowerVcmFeedbackHandle(void)
{
    stBlowerVcmFeedback lFeedback;
    uint16_t lSpeedScaled;
    uint8_t lSaturation;

    if (gBlowerVcmParseDataLength != BLOWER_VCM_FEEDBACK_DATA_LENGTH) {
        gBlowerVcmStats.rxLengthErrorCount++;
        return;
    }

    lSpeedScaled = (uint16_t)gBlowerVcmParseBuffer[3] |
                   ((uint16_t)gBlowerVcmParseBuffer[4] << 8U);
    lSaturation = gBlowerVcmParseBuffer[5];
    if (lSaturation > BLOWER_VCM_SATURATION_MAX) {
        gBlowerVcmStats.rxDataErrorCount++;
        return;
    }

    lFeedback.speedScaled = lSpeedScaled;
    lFeedback.speedRps = (float)lSpeedScaled / 10.0f;
    lFeedback.pcmSaturation = lSaturation;
    lFeedback.receivedAtMs = gBlowerVcmProcessNowMs;
    lFeedback.valid = true;
    repRtosEnterCritical();
    gBlowerVcmFeedback = lFeedback;
    repRtosExitCritical();
    gBlowerVcmStats.rxFrameCount++;
}

static void blowerVcmFrameHandle(void)
{
    uint16_t lExpectedCrc;
    uint16_t lReceivedCrc;
    uint8_t lCrcIndex = (uint8_t)(3U + gBlowerVcmParseDataLength);

    lExpectedCrc = blowerVcmCrc16(&gBlowerVcmParseBuffer[1],
                                 (uint8_t)(2U + gBlowerVcmParseDataLength));
    lReceivedCrc = (uint16_t)gBlowerVcmParseBuffer[lCrcIndex] |
                   ((uint16_t)gBlowerVcmParseBuffer[lCrcIndex + 1U] << 8U);
    if (lExpectedCrc != lReceivedCrc) {
        gBlowerVcmStats.rxCrcErrorCount++;
        return;
    }

    if (gBlowerVcmParseBuffer[1] == BLOWER_VCM_CMD_SPEED_FEEDBACK) {
        blowerVcmFeedbackHandle();
    } else {
        gBlowerVcmStats.rxUnknownCommandCount++;
    }
}

static void blowerVcmParseByte(uint8_t data)
{
    switch (gBlowerVcmParseState) {
        case BLOWER_VCM_PARSE_WAIT_HEADER:
            if (data == BLOWER_VCM_FRAME_HEADER) {
                gBlowerVcmParseBuffer[0] = data;
                gBlowerVcmParseIndex = 1U;
                gBlowerVcmParseState = BLOWER_VCM_PARSE_COMMAND;
            }
            break;

        case BLOWER_VCM_PARSE_COMMAND:
            /* Keep the newest header when PCM emits repeated synchronization bytes. */
            if (data == BLOWER_VCM_FRAME_HEADER) {
                gBlowerVcmParseBuffer[0] = data;
                break;
            }
            gBlowerVcmParseBuffer[gBlowerVcmParseIndex++] = data;
            gBlowerVcmParseState = BLOWER_VCM_PARSE_LENGTH;
            break;

        case BLOWER_VCM_PARSE_LENGTH:
            /* PCM may repeat the feedback command as part of its synchronization prefix. */
            if ((gBlowerVcmParseBuffer[1] == BLOWER_VCM_CMD_SPEED_FEEDBACK) &&
                (data == BLOWER_VCM_CMD_SPEED_FEEDBACK)) {
                break;
            }
            if (data > BLOWER_VCM_MAX_DATA_LENGTH) {
                gBlowerVcmStats.rxLengthErrorCount++;
                blowerVcmParserReset(data);
                break;
            }
            gBlowerVcmParseBuffer[gBlowerVcmParseIndex++] = data;
            gBlowerVcmParseDataLength = data;
            gBlowerVcmParseState = (data == 0U) ? BLOWER_VCM_PARSE_CRC_LOW : BLOWER_VCM_PARSE_DATA;
            break;

        case BLOWER_VCM_PARSE_DATA:
            gBlowerVcmParseBuffer[gBlowerVcmParseIndex++] = data;
            if (gBlowerVcmParseIndex == (uint8_t)(3U + gBlowerVcmParseDataLength)) {
                gBlowerVcmParseState = BLOWER_VCM_PARSE_CRC_LOW;
            }
            break;

        case BLOWER_VCM_PARSE_CRC_LOW:
            gBlowerVcmParseBuffer[gBlowerVcmParseIndex++] = data;
            gBlowerVcmParseState = BLOWER_VCM_PARSE_CRC_HIGH;
            break;

        case BLOWER_VCM_PARSE_CRC_HIGH:
            gBlowerVcmParseBuffer[gBlowerVcmParseIndex++] = data;
            gBlowerVcmParseState = BLOWER_VCM_PARSE_TAIL;
            break;

        case BLOWER_VCM_PARSE_TAIL:
            if (data == BLOWER_VCM_FRAME_TAIL) {
                blowerVcmFrameHandle();
                blowerVcmParserReset(0U);
            } else {
                gBlowerVcmStats.rxTailErrorCount++;
                blowerVcmParserReset(data);
            }
            break;

        default:
            blowerVcmParserReset(data);
            break;
    }
}

static int8_t blowerVcmProtocolSelfTest(void)
{
    static const uint8_t lCrcVector[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    uint8_t lFrame[BLOWER_VCM_FEEDBACK_DATA_LENGTH + BLOWER_VCM_FRAME_OVERHEAD + 2U];
    uint16_t lCrc;
    uint8_t lIndex;

    if (blowerVcmCrc16(lCrcVector, (uint8_t)sizeof(lCrcVector)) != 0x4B37U) {
        return BLOWER_VCM_ERROR_SELF_TEST;
    }

    lFrame[0] = BLOWER_VCM_FRAME_HEADER;
    lFrame[1] = BLOWER_VCM_FRAME_HEADER;
    lFrame[2] = BLOWER_VCM_CMD_SPEED_FEEDBACK;
    lFrame[3] = BLOWER_VCM_CMD_SPEED_FEEDBACK;
    lFrame[4] = BLOWER_VCM_FEEDBACK_DATA_LENGTH;
    lFrame[5] = 0xD2U;
    lFrame[6] = 0x04U;
    lFrame[7] = 1U;
    lCrc = blowerVcmCrc16(&lFrame[3], 2U + BLOWER_VCM_FEEDBACK_DATA_LENGTH);
    lFrame[8] = (uint8_t)(lCrc & 0xFFU);
    lFrame[9] = (uint8_t)(lCrc >> 8U);
    lFrame[10] = BLOWER_VCM_FRAME_TAIL;
    gBlowerVcmProcessNowMs = 42U;
    for (lIndex = 0U; lIndex < (uint8_t)sizeof(lFrame); lIndex++) {
        blowerVcmParseByte(lFrame[lIndex]);
    }
    if ((!gBlowerVcmFeedback.valid) ||
        (gBlowerVcmFeedback.speedScaled != 1234U) ||
        (gBlowerVcmFeedback.pcmSaturation != 1U) ||
        (gBlowerVcmFeedback.receivedAtMs != 42U) ||
        (gBlowerVcmStats.rxFrameCount != 1U)) {
        return BLOWER_VCM_ERROR_SELF_TEST;
    }
    return BLOWER_VCM_STATUS_OK;
}

static void blowerVcmRxPush(uint8_t data)
{
    uint16_t lNextWrite = (uint16_t)((gBlowerVcmRxWrite + 1U) & BLOWER_VCM_RX_FIFO_MASK);

    gBlowerVcmStats.rxByteCount++;
    if (lNextWrite == gBlowerVcmRxRead) {
        gBlowerVcmStats.rxOverflowCount++;
        gBlowerVcmRxOverflow = true;
        return;
    }
    gBlowerVcmRxFifo[gBlowerVcmRxWrite] = data;
    __DMB();
    gBlowerVcmRxWrite = lNextWrite;
}

static void blowerVcmDmaCollect(void)
{
    uint16_t lRemaining;
    uint16_t lWritePos;
    uint16_t lReadPos;

    if (gBlowerVcmDmaCollecting) {
        return;
    }
    gBlowerVcmDmaCollecting = true;
    __DMB();
    lRemaining = (uint16_t)dma_transfer_number_get(DMA0, DMA_CH0);
    lWritePos = (uint16_t)(BLOWER_VCM_RX_DMA_SIZE - lRemaining);
    lReadPos = gBlowerVcmDmaReadPos;

    if (lWritePos >= BLOWER_VCM_RX_DMA_SIZE) {
        lWritePos = 0U;
    }
    while (lReadPos != lWritePos) {
        blowerVcmRxPush(gBlowerVcmRxDmaBuffer[lReadPos]);
        lReadPos++;
        if (lReadPos == BLOWER_VCM_RX_DMA_SIZE) {
            lReadPos = 0U;
        }
    }
    gBlowerVcmDmaReadPos = lReadPos;
    __DMB();
    gBlowerVcmDmaCollecting = false;
}

static void blowerVcmGpioInit(void)
{
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
    gpio_af_set(GPIOC, GPIO_AF_8, GPIO_PIN_12);
    gpio_af_set(GPIOD, GPIO_AF_8, GPIO_PIN_2);
    gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_12);
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_2);
    gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_12);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
}

static void blowerVcmDmaInit(void)
{
    dma_single_data_parameter_struct lDmaConfig;

    rcu_periph_clock_enable(RCU_DMA0);
    dma_channel_disable(DMA0, DMA_CH0);
    dma_deinit(DMA0, DMA_CH0);
    dma_single_data_para_struct_init(&lDmaConfig);
    lDmaConfig.periph_addr = (uint32_t)&USART_DATA(UART4);
    lDmaConfig.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    lDmaConfig.memory0_addr = (uint32_t)gBlowerVcmRxDmaBuffer;
    lDmaConfig.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    lDmaConfig.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    lDmaConfig.circular_mode = DMA_CIRCULAR_MODE_ENABLE;
    lDmaConfig.direction = DMA_PERIPH_TO_MEMORY;
    lDmaConfig.number = BLOWER_VCM_RX_DMA_SIZE;
    lDmaConfig.priority = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(DMA0, DMA_CH0, &lDmaConfig);
    dma_channel_subperipheral_select(DMA0, DMA_CH0, DMA_SUBPERI4);
    dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_FEE);
    dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_SDE);
    dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_TAE);
    dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_HTF);
    dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_FTF);
    dma_interrupt_enable(DMA0, DMA_CH0, DMA_CHXCTL_SDEIE);
    dma_interrupt_enable(DMA0, DMA_CH0, DMA_CHXCTL_TAEIE);
    dma_interrupt_enable(DMA0, DMA_CH0, DMA_CHXCTL_HTFIE);
    dma_interrupt_enable(DMA0, DMA_CH0, DMA_CHXCTL_FTFIE);
    dma_interrupt_enable(DMA0, DMA_CH0, DMA_CHXFCTL_FEEIE);

    dma_channel_disable(DMA0, DMA_CH7);
    dma_deinit(DMA0, DMA_CH7);
    dma_single_data_para_struct_init(&lDmaConfig);
    lDmaConfig.periph_addr = (uint32_t)&USART_DATA(UART4);
    lDmaConfig.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    lDmaConfig.memory0_addr = (uint32_t)gBlowerVcmTxBuffer;
    lDmaConfig.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    lDmaConfig.periph_memory_width = DMA_PERIPH_WIDTH_8BIT;
    lDmaConfig.circular_mode = DMA_CIRCULAR_MODE_DISABLE;
    lDmaConfig.direction = DMA_MEMORY_TO_PERIPH;
    lDmaConfig.number = BLOWER_VCM_TX_DMA_SIZE;
    lDmaConfig.priority = DMA_PRIORITY_HIGH;
    dma_single_data_mode_init(DMA0, DMA_CH7, &lDmaConfig);
    dma_channel_subperipheral_select(DMA0, DMA_CH7, DMA_SUBPERI4);
    dma_interrupt_flag_clear(DMA0, DMA_CH7, DMA_INT_FLAG_FEE);
    dma_interrupt_flag_clear(DMA0, DMA_CH7, DMA_INT_FLAG_SDE);
    dma_interrupt_flag_clear(DMA0, DMA_CH7, DMA_INT_FLAG_TAE);
    dma_interrupt_flag_clear(DMA0, DMA_CH7, DMA_INT_FLAG_FTF);
    dma_interrupt_enable(DMA0, DMA_CH7, DMA_CHXCTL_SDEIE);
    dma_interrupt_enable(DMA0, DMA_CH7, DMA_CHXCTL_TAEIE);
    dma_interrupt_enable(DMA0, DMA_CH7, DMA_CHXCTL_FTFIE);
    dma_interrupt_enable(DMA0, DMA_CH7, DMA_CHXFCTL_FEEIE);
}

static void blowerVcmUartInit(void)
{
    rcu_periph_clock_enable(RCU_UART4);
    usart_deinit(UART4);
    usart_baudrate_set(UART4, BLOWER_VCM_BAUDRATE);
    usart_word_length_set(UART4, USART_WL_8BIT);
    usart_stop_bit_set(UART4, USART_STB_1BIT);
    usart_parity_config(UART4, USART_PM_NONE);
    usart_hardware_flow_rts_config(UART4, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(UART4, USART_CTS_DISABLE);
    usart_receive_config(UART4, USART_RECEIVE_ENABLE);
    usart_transmit_config(UART4, USART_TRANSMIT_ENABLE);
    usart_interrupt_enable(UART4, USART_INT_IDLE);
    usart_interrupt_enable(UART4, USART_INT_ERR);
    usart_dma_receive_config(UART4, USART_RECEIVE_DMA_ENABLE);
}

int8_t blowerVcmInit(void)
{
    if (gBlowerVcmInitialized) {
        return BLOWER_VCM_STATUS_OK;
    }

    blowerVcmMemoryClear(gBlowerVcmRxDmaBuffer, BLOWER_VCM_RX_DMA_SIZE);
    blowerVcmMemoryClear(gBlowerVcmRxFifo, BLOWER_VCM_RX_FIFO_SIZE);
    blowerVcmMemoryClear(gBlowerVcmTxBuffer, BLOWER_VCM_TX_DMA_SIZE);
    blowerVcmMemoryClear(gBlowerVcmParseBuffer, BLOWER_VCM_MAX_FRAME_LENGTH);
    blowerVcmStatsClear();
    gBlowerVcmRxWrite = 0U;
    gBlowerVcmRxRead = 0U;
    gBlowerVcmDmaReadPos = 0U;
    gBlowerVcmTxBusy = false;
    gBlowerVcmRxOverflow = false;
    gBlowerVcmDmaCollecting = false;
    gBlowerVcmLastControlValid = false;
    gBlowerVcmLastTxAtMs = 0U;
    gBlowerVcmFeedback.speedRps = 0.0f;
    gBlowerVcmFeedback.speedScaled = 0U;
    gBlowerVcmFeedback.pcmSaturation = 0U;
    gBlowerVcmFeedback.receivedAtMs = 0U;
    gBlowerVcmFeedback.valid = false;
    blowerVcmParserReset(0U);
    if (blowerVcmProtocolSelfTest() != BLOWER_VCM_STATUS_OK) {
        gBlowerVcmFeedback.valid = false;
        blowerVcmStatsClear();
        blowerVcmParserReset(0U);
        return BLOWER_VCM_ERROR_SELF_TEST;
    }
    gBlowerVcmFeedback.speedRps = 0.0f;
    gBlowerVcmFeedback.speedScaled = 0U;
    gBlowerVcmFeedback.pcmSaturation = 0U;
    gBlowerVcmFeedback.receivedAtMs = 0U;
    gBlowerVcmFeedback.valid = false;
    gBlowerVcmProcessNowMs = 0U;
    blowerVcmStatsClear();
    blowerVcmParserReset(0U);

    blowerVcmGpioInit();
    blowerVcmDmaInit();
    blowerVcmUartInit();
    NVIC_ClearPendingIRQ(UART4_IRQn);
    NVIC_ClearPendingIRQ(DMA0_Channel0_IRQn);
    NVIC_ClearPendingIRQ(DMA0_Channel7_IRQn);
    NVIC_SetPriority(UART4_IRQn, 6U);
    NVIC_SetPriority(DMA0_Channel0_IRQn, 7U);
    NVIC_SetPriority(DMA0_Channel7_IRQn, 7U);
    NVIC_EnableIRQ(UART4_IRQn);
    NVIC_EnableIRQ(DMA0_Channel0_IRQn);
    NVIC_EnableIRQ(DMA0_Channel7_IRQn);
    dma_channel_enable(DMA0, DMA_CH0);
    usart_enable(UART4);
    gBlowerVcmInitialized = true;
    return BLOWER_VCM_STATUS_OK;
}

void blowerVcmDeInit(void)
{
    if (!gBlowerVcmInitialized) {
        return;
    }

    NVIC_DisableIRQ(UART4_IRQn);
    NVIC_DisableIRQ(DMA0_Channel0_IRQn);
    NVIC_DisableIRQ(DMA0_Channel7_IRQn);
    usart_interrupt_disable(UART4, USART_INT_TBE);
    usart_interrupt_disable(UART4, USART_INT_TC);
    usart_interrupt_disable(UART4, USART_INT_IDLE);
    usart_interrupt_disable(UART4, USART_INT_ERR);
    usart_dma_receive_config(UART4, USART_RECEIVE_DMA_DISABLE);
    usart_dma_transmit_config(UART4, USART_TRANSMIT_DMA_DISABLE);
    dma_channel_disable(DMA0, DMA_CH0);
    dma_channel_disable(DMA0, DMA_CH7);
    dma_deinit(DMA0, DMA_CH0);
    dma_deinit(DMA0, DMA_CH7);
    usart_disable(UART4);
    usart_deinit(UART4);
    NVIC_ClearPendingIRQ(UART4_IRQn);
    NVIC_ClearPendingIRQ(DMA0_Channel0_IRQn);
    NVIC_ClearPendingIRQ(DMA0_Channel7_IRQn);
    gBlowerVcmTxBusy = false;
    gBlowerVcmInitialized = false;
}

int8_t blowerVcmProcess(uint32_t nowMs)
{
    uint16_t lCount = 0U;
    uint8_t lData;

    if (!gBlowerVcmInitialized) {
        return BLOWER_VCM_ERROR_NOT_READY;
    }
    if (gBlowerVcmRxOverflow) {
        repRtosEnterCritical();
        gBlowerVcmRxRead = gBlowerVcmRxWrite;
        gBlowerVcmRxOverflow = false;
        repRtosExitCritical();
        blowerVcmParserReset(0U);
    }
    gBlowerVcmProcessNowMs = nowMs;
    while ((gBlowerVcmRxRead != gBlowerVcmRxWrite) &&
           (lCount < BLOWER_VCM_PROCESS_BYTE_LIMIT)) {
        lData = gBlowerVcmRxFifo[gBlowerVcmRxRead];
        gBlowerVcmRxRead = (uint16_t)((gBlowerVcmRxRead + 1U) & BLOWER_VCM_RX_FIFO_MASK);
        blowerVcmParseByte(lData);
        lCount++;
    }
    if (gBlowerVcmLastControlValid && !gBlowerVcmTxBusy &&
        ((uint32_t)(nowMs - gBlowerVcmLastTxAtMs) >= BLOWER_VCM_CONTROL_KEEPALIVE_MS)) {
        (void)blowerVcmSendControl(gBlowerVcmLastMode,
                                  gBlowerVcmLastTargetValue,
                                  gBlowerVcmLastSaturation);
    }
    return BLOWER_VCM_STATUS_OK;
}

int8_t blowerVcmSendControl(eBlowerVcmControlMode mode, uint16_t targetValue, uint8_t vcmSaturation)
{
    uint8_t lFrame[BLOWER_VCM_CONTROL_DATA_LENGTH + BLOWER_VCM_FRAME_OVERHEAD];
    uint16_t lCrc;
    uint8_t lFrameIndex;
    uint8_t lRepeatIndex;

    if (((uint32_t)mode >= (uint32_t)BLOWER_CTRL_MAX) ||
        (vcmSaturation > BLOWER_VCM_SATURATION_MAX)) {
        return BLOWER_VCM_ERROR_INVALID_PARAM;
    }
    if (!gBlowerVcmInitialized) {
        return BLOWER_VCM_ERROR_NOT_READY;
    }

    lFrame[0] = BLOWER_VCM_FRAME_HEADER;
    lFrame[1] = BLOWER_VCM_CMD_CONTROL;
    lFrame[2] = BLOWER_VCM_CONTROL_DATA_LENGTH;
    lFrame[3] = (uint8_t)mode;
    lFrame[4] = (uint8_t)(targetValue & 0xFFU);
    lFrame[5] = (uint8_t)(targetValue >> 8U);
    lFrame[6] = vcmSaturation;
    lCrc = blowerVcmCrc16(&lFrame[1], 2U + BLOWER_VCM_CONTROL_DATA_LENGTH);
    lFrame[7] = (uint8_t)(lCrc & 0xFFU);
    lFrame[8] = (uint8_t)(lCrc >> 8U);
    lFrame[9] = BLOWER_VCM_FRAME_TAIL;

    repRtosEnterCritical();
    if (gBlowerVcmLastControlValid &&
        (gBlowerVcmLastMode == mode) &&
        (gBlowerVcmLastTargetValue == targetValue) &&
        (gBlowerVcmLastSaturation == vcmSaturation) &&
        ((uint32_t)(gBlowerVcmProcessNowMs - gBlowerVcmLastTxAtMs) < BLOWER_VCM_CONTROL_KEEPALIVE_MS)) {
        repRtosExitCritical();
        return BLOWER_VCM_STATUS_OK;
    }
    if (gBlowerVcmTxBusy) {
        gBlowerVcmStats.txBusyDropCount++;
        repRtosExitCritical();
        return BLOWER_VCM_ERROR_BUSY;
    }
    for (lRepeatIndex = 0U; lRepeatIndex < BLOWER_VCM_CONTROL_FRAME_COUNT; lRepeatIndex++) {
        for (lFrameIndex = 0U; lFrameIndex < (uint8_t)sizeof(lFrame); lFrameIndex++) {
            gBlowerVcmTxBuffer[(lRepeatIndex * (uint8_t)sizeof(lFrame)) + lFrameIndex] = lFrame[lFrameIndex];
        }
    }
    dma_channel_disable(DMA0, DMA_CH7);
    dma_interrupt_flag_clear(DMA0, DMA_CH7, DMA_INT_FLAG_FEE);
    dma_interrupt_flag_clear(DMA0, DMA_CH7, DMA_INT_FLAG_SDE);
    dma_interrupt_flag_clear(DMA0, DMA_CH7, DMA_INT_FLAG_TAE);
    dma_interrupt_flag_clear(DMA0, DMA_CH7, DMA_INT_FLAG_FTF);
    dma_memory_address_config(DMA0, DMA_CH7, DMA_MEMORY_0, (uint32_t)gBlowerVcmTxBuffer);
    dma_transfer_number_config(DMA0, DMA_CH7, BLOWER_VCM_TX_DMA_SIZE);
    gBlowerVcmTxBusy = true;
    gBlowerVcmLastControlValid = true;
    gBlowerVcmLastMode = mode;
    gBlowerVcmLastTargetValue = targetValue;
    gBlowerVcmLastSaturation = vcmSaturation;
    gBlowerVcmLastTxAtMs = gBlowerVcmProcessNowMs;
    usart_flag_clear(UART4, USART_FLAG_TC);
    dma_channel_enable(DMA0, DMA_CH7);
    usart_dma_transmit_config(UART4, USART_TRANSMIT_DMA_ENABLE);
    repRtosExitCritical();
    return BLOWER_VCM_STATUS_OK;
}

bool blowerVcmIsConnected(uint32_t nowMs)
{
    return gBlowerVcmFeedback.valid &&
           ((uint32_t)(nowMs - gBlowerVcmFeedback.receivedAtMs) < BLOWER_VCM_FEEDBACK_TIMEOUT_MS);
}

bool blowerVcmIsTxBusy(void)
{
    return gBlowerVcmTxBusy;
}

int8_t blowerVcmGetFeedback(stBlowerVcmFeedback *feedback)
{
    if (feedback == NULL) {
        return BLOWER_VCM_ERROR_INVALID_PARAM;
    }
    repRtosEnterCritical();
    *feedback = gBlowerVcmFeedback;
    repRtosExitCritical();
    return BLOWER_VCM_STATUS_OK;
}

int8_t blowerVcmGetStats(stBlowerVcmStats *stats)
{
    if (stats == NULL) {
        return BLOWER_VCM_ERROR_INVALID_PARAM;
    }
    repRtosEnterCritical();
    *stats = gBlowerVcmStats;
    repRtosExitCritical();
    return BLOWER_VCM_STATUS_OK;
}

void UART4_IRQHandler(void)
{
    uint32_t lStatus = USART_STAT0(UART4);
    volatile uint32_t lDiscard;

    if ((lStatus & USART_STAT0_IDLEF) != 0U) {
        lDiscard = USART_STAT0(UART4);
        lDiscard = USART_DATA(UART4);
        (void)lDiscard;
        __DMB();
        blowerVcmDmaCollect();
    }
    if ((lStatus & (USART_STAT0_ORERR | USART_STAT0_NERR | USART_STAT0_FERR | USART_STAT0_PERR)) != 0U) {
        gBlowerVcmStats.uartErrorCount++;
        lDiscard = USART_STAT0(UART4);
        lDiscard = USART_DATA(UART4);
        (void)lDiscard;
    }
    if (usart_interrupt_flag_get(UART4, USART_INT_FLAG_TC) == SET) {
        usart_interrupt_disable(UART4, USART_INT_TC);
        usart_interrupt_flag_clear(UART4, USART_INT_FLAG_TC);
        gBlowerVcmTxBusy = false;
        gBlowerVcmStats.txFrameCount += BLOWER_VCM_CONTROL_FRAME_COUNT;
    }
}

void DMA0_Channel0_IRQHandler(void)
{
    bool lTransferEvent = false;

    if (dma_interrupt_flag_get(DMA0, DMA_CH0, DMA_INT_FLAG_HTF) == SET) {
        dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_HTF);
        lTransferEvent = true;
    }
    if (dma_interrupt_flag_get(DMA0, DMA_CH0, DMA_INT_FLAG_FTF) == SET) {
        dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_FTF);
        lTransferEvent = true;
    }
    if (lTransferEvent) {
        blowerVcmDmaCollect();
    }
    if (dma_interrupt_flag_get(DMA0, DMA_CH0, DMA_INT_FLAG_FEE) == SET) {
        dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_FEE);
        gBlowerVcmStats.dmaErrorCount++;
    }
    if (dma_interrupt_flag_get(DMA0, DMA_CH0, DMA_INT_FLAG_SDE) == SET) {
        dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_SDE);
        gBlowerVcmStats.dmaErrorCount++;
    }
    if (dma_interrupt_flag_get(DMA0, DMA_CH0, DMA_INT_FLAG_TAE) == SET) {
        dma_interrupt_flag_clear(DMA0, DMA_CH0, DMA_INT_FLAG_TAE);
        gBlowerVcmStats.dmaErrorCount++;
    }
}

void DMA0_Channel7_IRQHandler(void)
{
    bool lError = false;

    if (dma_interrupt_flag_get(DMA0, DMA_CH7, DMA_INT_FLAG_FEE) == SET) {
        dma_interrupt_flag_clear(DMA0, DMA_CH7, DMA_INT_FLAG_FEE);
        lError = true;
    }
    if (dma_interrupt_flag_get(DMA0, DMA_CH7, DMA_INT_FLAG_SDE) == SET) {
        dma_interrupt_flag_clear(DMA0, DMA_CH7, DMA_INT_FLAG_SDE);
        lError = true;
    }
    if (dma_interrupt_flag_get(DMA0, DMA_CH7, DMA_INT_FLAG_TAE) == SET) {
        dma_interrupt_flag_clear(DMA0, DMA_CH7, DMA_INT_FLAG_TAE);
        lError = true;
    }
    if (lError) {
        dma_channel_disable(DMA0, DMA_CH7);
        usart_dma_transmit_config(UART4, USART_TRANSMIT_DMA_DISABLE);
        gBlowerVcmStats.dmaErrorCount++;
        gBlowerVcmTxBusy = false;
        usart_interrupt_disable(UART4, USART_INT_TC);
        return;
    }
    if (dma_interrupt_flag_get(DMA0, DMA_CH7, DMA_INT_FLAG_FTF) == SET) {
        dma_interrupt_flag_clear(DMA0, DMA_CH7, DMA_INT_FLAG_FTF);
        dma_channel_disable(DMA0, DMA_CH7);
        usart_dma_transmit_config(UART4, USART_TRANSMIT_DMA_DISABLE);
        usart_interrupt_enable(UART4, USART_INT_TC);
    }
}

/**************************End of file********************************/
