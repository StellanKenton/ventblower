/************************************************************************************
* @file     : blower_vcm.h
* @brief    : VCM blower UART communication BSP.
* @details  : Declares protocol, reliable control TX, feedback, and diagnostics.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef USER_BSP_BLOWER_VCM_H
#define USER_BSP_BLOWER_VCM_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BLOWER_VCM_STATUS_OK                    ((int8_t)1)
#define BLOWER_VCM_ERROR_INVALID_PARAM          ((int8_t)-1)
#define BLOWER_VCM_ERROR_NOT_READY              ((int8_t)-2)
#define BLOWER_VCM_ERROR_BUSY                   ((int8_t)-3)
#define BLOWER_VCM_ERROR_SELF_TEST              ((int8_t)-4)

#define BLOWER_VCM_FRAME_HEADER                 0xAAU
#define BLOWER_VCM_FRAME_TAIL                   0x55U
#define BLOWER_VCM_CMD_CONTROL                  0x01U
#define BLOWER_VCM_CMD_SPEED_FEEDBACK           0x02U
#define BLOWER_VCM_CONTROL_DATA_LENGTH          4U
#define BLOWER_VCM_FEEDBACK_DATA_LENGTH         3U
#define BLOWER_VCM_MAX_DATA_LENGTH              8U
#define BLOWER_VCM_FRAME_OVERHEAD               6U
#define BLOWER_VCM_MAX_FRAME_LENGTH             (BLOWER_VCM_MAX_DATA_LENGTH + BLOWER_VCM_FRAME_OVERHEAD)
#define BLOWER_VCM_RX_DMA_SIZE                  256U
#define BLOWER_VCM_RX_FIFO_SIZE                 512U
#define BLOWER_VCM_RX_FIFO_MASK                 (BLOWER_VCM_RX_FIFO_SIZE - 1U)
#define BLOWER_VCM_PROCESS_BYTE_LIMIT           BLOWER_VCM_RX_FIFO_SIZE
#define BLOWER_VCM_FEEDBACK_TIMEOUT_MS          1000U
/* Repeat unchanged targets promptly, including the zero-speed release command. */
#define BLOWER_VCM_CONTROL_KEEPALIVE_MS           10U
#define BLOWER_VCM_BAUDRATE                     230400U
#define BLOWER_VCM_SATURATION_MAX               1U
#define BLOWER_VCM_CONTROL_FRAME_COUNT           2U
#define BLOWER_VCM_TX_DMA_SIZE                   \
    ((BLOWER_VCM_CONTROL_DATA_LENGTH + BLOWER_VCM_FRAME_OVERHEAD) * BLOWER_VCM_CONTROL_FRAME_COUNT)

#define BLOWER_VCM_PARSE_WAIT_HEADER            0U
#define BLOWER_VCM_PARSE_COMMAND                1U
#define BLOWER_VCM_PARSE_LENGTH                 2U
#define BLOWER_VCM_PARSE_DATA                   3U
#define BLOWER_VCM_PARSE_CRC_LOW                4U
#define BLOWER_VCM_PARSE_CRC_HIGH               5U
#define BLOWER_VCM_PARSE_TAIL                   6U

typedef enum eBlowerVcmControlMode {
    BLOWER_CTRL_INIT = 0,
    BLOWER_CTRL_PWM,
    BLOWER_CTRL_CURRENT_LIMIT,
    BLOWER_CTRL_CURRENT,
    BLOWER_CTRL_CURRENT_LIMIT_SPEED,
    BLOWER_CTRL_CURRENT_SPEED,
    BLOWER_CTRL_SPEED,
    BLOWER_CTRL_MAX,
} eBlowerVcmControlMode;

typedef struct stBlowerVcmFeedback {
    float speedRps;
    uint16_t speedScaled;
    uint8_t pcmSaturation;
    uint32_t receivedAtMs;
    bool valid;
} stBlowerVcmFeedback;

typedef struct stBlowerVcmStats {
    uint32_t rxByteCount;
    uint32_t rxFrameCount;
    uint32_t rxCrcErrorCount;
    uint32_t rxLengthErrorCount;
    uint32_t rxTailErrorCount;
    uint32_t rxDataErrorCount;
    uint32_t rxUnknownCommandCount;
    uint32_t rxOverflowCount;
    uint32_t uartErrorCount;
    uint32_t dmaErrorCount;
    uint32_t txFrameCount;
    uint32_t txBusyDropCount;
} stBlowerVcmStats;

/**
 * @brief Initialize UART4 and its circular RX DMA for the VCM blower link.
 * @return BLOWER_VCM_STATUS_OK on success.
 * @note Call once from SensorTask context before blowerVcmProcess().
 */
int8_t blowerVcmInit(void);

/**
 * @brief Disable the VCM blower UART, DMA, and interrupts.
 */
void blowerVcmDeInit(void);

/**
 * @brief Consume received bytes and update blower feedback.
 * @param nowMs Current monotonic system time in milliseconds.
 * @return BLOWER_VCM_STATUS_OK, or BLOWER_VCM_ERROR_NOT_READY.
 * @note Call periodically from SensorTask context.
 */
int8_t blowerVcmProcess(uint32_t nowMs);

/**
 * @brief Send one blower control frame.
 * @param mode PCM control mode encoded on the wire.
 * @param targetScaled Target already converted to the protocol scale.
 * @param vcmSaturation VCM saturation flag, either 0 or 1.
 * @return BLOWER_VCM_STATUS_OK, or a negative error code.
 * @note Call from task context. Changed settings are sent immediately and the
 * latest setting is periodically retransmitted by blowerVcmProcess().
 */
int8_t blowerVcmSendControl(eBlowerVcmControlMode mode, uint16_t targetValue, uint8_t vcmSaturation);

/**
 * @brief Check whether valid feedback was received within the timeout.
 * @param nowMs Current monotonic system time in milliseconds.
 * @return true only after at least one recent valid speed feedback frame.
 */
bool blowerVcmIsConnected(uint32_t nowMs);

/**
 * @brief Check whether a control frame is being transmitted.
 * @return true while the UART owns the private TX buffer.
 */
bool blowerVcmIsTxBusy(void);

/**
 * @brief Copy the latest decoded blower feedback.
 * @param feedback Destination for the feedback snapshot.
 * @return BLOWER_VCM_STATUS_OK, or BLOWER_VCM_ERROR_INVALID_PARAM.
 */
int8_t blowerVcmGetFeedback(stBlowerVcmFeedback *feedback);

/**
 * @brief Copy communication diagnostic counters.
 * @param stats Destination for the diagnostic snapshot.
 * @return BLOWER_VCM_STATUS_OK, or BLOWER_VCM_ERROR_INVALID_PARAM.
 */
int8_t blowerVcmGetStats(stBlowerVcmStats *stats);

/** @brief UART4 interrupt entry point. */
void UART4_IRQHandler(void);

/** @brief DMA0 channel 0 interrupt entry point. */
void DMA0_Channel0_IRQHandler(void);

/** @brief DMA0 channel 7 interrupt entry point. */
void DMA0_Channel7_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_BSP_BLOWER_VCM_H */
/**************************End of file********************************/
