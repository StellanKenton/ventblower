/************************************************************************************
* @file     : ringbuffer.h
* @brief    : Byte-oriented SPSC ring buffer public API.
* @details  : This module provides a reusable ring buffer core for MCU projects.
* @author   : GitHub Copilot
* @date     : 2026-03-30
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdint.h>

#define RING_USE_RTOS 1

#ifdef __cplusplus
extern "C" {
#endif

#define RINGBUFFER_MAX_CAPACITY    (UINT32_MAX / 2U)


typedef enum eRingBufferStatus {
    RINGBUFFER_OK = 0,
    RINGBUFFER_ERROR_PARAM,
    RINGBUFFER_ERROR_EMPTY,
    RINGBUFFER_ERROR_FULL,
    RINGBUFFER_ERROR_NO_SPACE,
    RINGBUFFER_ERROR_STATE
} eRingBufferStatus;

typedef struct stRingBuffer {
    uint8_t *buffer;
    uint32_t capacity;
    volatile uint32_t head;
    volatile uint32_t tail;
    uint32_t mask;
    uint8_t isPowerOfTwo;
} stRingBuffer;

typedef struct stRingBufferOps {
    void (*enterCritical)(void);
    void (*exitCritical)(void);
    void (*memoryBarrier)(void);
} stRingBufferOps;

eRingBufferStatus ringBufferInit(stRingBuffer *rb, uint8_t *storage, uint32_t capacity);
eRingBufferStatus ringBufferReset(stRingBuffer *rb);

uint32_t ringBufferGetUsed(const stRingBuffer *rb);
uint32_t ringBufferGetFree(const stRingBuffer *rb);
uint32_t ringBufferGetCapacity(const stRingBuffer *rb);
uint8_t ringBufferIsEmpty(const stRingBuffer *rb);
uint8_t ringBufferIsFull(const stRingBuffer *rb);

eRingBufferStatus ringBufferPushByte(stRingBuffer *rb, uint8_t data);
eRingBufferStatus ringBufferPopByte(stRingBuffer *rb, uint8_t *data);
eRingBufferStatus ringBufferPeekByte(const stRingBuffer *rb, uint8_t *data);

uint32_t ringBufferWrite(stRingBuffer *rb, const uint8_t *src, uint32_t length);
uint32_t ringBufferRead(stRingBuffer *rb, uint8_t *dst, uint32_t length);
uint32_t ringBufferPeek(const stRingBuffer *rb, uint8_t *dst, uint32_t length);
uint32_t ringBufferDiscard(stRingBuffer *rb, uint32_t length);

/* This API changes both indices and therefore requires exclusive access. */
uint32_t ringBufferWriteOverwrite(stRingBuffer *rb, const uint8_t *src, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif  // RINGBUFFER_H
/**************************End of file********************************/
