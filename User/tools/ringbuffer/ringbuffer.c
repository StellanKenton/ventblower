/***********************************************************************************
* @file     : ringbuffer.c
* @brief    : Byte-oriented SPSC ring buffer implementation.
* @details  : The core logic is portable and does not depend on an RTOS.
* @author   : GitHub Copilot
* @date     : 2026-03-30
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "ringbuffer.h"
#include <string.h>
#if RING_USE_RTOS
#include "rtos.h"
#endif

static const stRingBufferOps *ringBufferGetOps(void);

static const stRingBufferOps *ringBufferGetOps(void)
{
#if RING_USE_RTOS
    static const stRingBufferOps lOps = {
        .enterCritical = repRtosEnterCritical,
        .exitCritical = repRtosExitCritical,
        .memoryBarrier = NULL,
    };

    return &lOps;
#else
    return NULL;
#endif
}

static void ringBufferEnterCritical(void)
{
    const stRingBufferOps *lOps = ringBufferGetOps();

    if ((lOps != NULL) && (lOps->enterCritical != NULL)) {
        lOps->enterCritical();
    }
}

static void ringBufferExitCritical(void)
{
    const stRingBufferOps *lOps = ringBufferGetOps();

    if ((lOps != NULL) && (lOps->exitCritical != NULL)) {
        lOps->exitCritical();
    }
}

static void ringBufferMemoryBarrier(void)
{
    const stRingBufferOps *lOps = ringBufferGetOps();

    if ((lOps != NULL) && (lOps->memoryBarrier != NULL)) {
        lOps->memoryBarrier();
    }
}
/**
* @brief : Check whether the requested capacity is a power of two.
* @param : capacity Ring buffer capacity in bytes.
* @return: Non-zero when the capacity is a power of two, otherwise zero.
**/
static uint8_t ringBufferIsPowerOfTwoInternal(uint32_t capacity)
{
    return (uint8_t)((capacity != 0U) && ((capacity & (capacity - 1U)) == 0U));
}

/**
* @brief : Verify that the ring buffer control block has valid basic configuration.
* @param : rb Pointer to the ring buffer control block.
* @return: Non-zero when the control block is initialized, otherwise zero.
**/
static uint8_t ringBufferIsConfigured(const stRingBuffer *rb)
{
    return (uint8_t)((rb != NULL) && (rb->buffer != NULL) && (rb->capacity != 0U));
}

/**
* @brief : Get the number of bytes currently stored in the ring buffer.
* @param : rb Pointer to the ring buffer control block.
* @return: Used byte count derived from the logical indices.
**/
static uint32_t ringBufferGetUsedInternal(const stRingBuffer *rb)
{
    return rb->head - rb->tail;
}

/**
* @brief : Check whether the ring buffer indices remain within a valid range.
* @param : rb Pointer to the ring buffer control block.
* @return: Non-zero when the ring buffer state is valid, otherwise zero.
**/
static uint8_t ringBufferHasValidState(const stRingBuffer *rb)
{
    if (ringBufferIsConfigured(rb) == 0U) {
        return 0U;
    }

    return (uint8_t)(ringBufferGetUsedInternal(rb) <= rb->capacity);
}

/**
* @brief : Convert a logical index to the corresponding physical buffer offset.
* @param : rb           Pointer to the ring buffer control block.
* @param : logicalIndex Monotonic logical index.
* @return: Physical array index within the storage buffer.
**/
static uint32_t ringBufferToPhysicalIndex(const stRingBuffer *rb, uint32_t logicalIndex)
{
    if (rb->isPowerOfTwo != 0U) {
        return logicalIndex & rb->mask;
    }

    return logicalIndex % rb->capacity;
}

/**
* @brief : Get the largest contiguous write span before wrap-around.
* @param : rb Pointer to the ring buffer control block.
* @return: Number of bytes that can be written linearly.
**/
static uint32_t ringBufferGetLinearWriteSpan(const stRingBuffer *rb)
{
    uint32_t lFree = rb->capacity - ringBufferGetUsedInternal(rb);
    uint32_t lPhysicalHead = ringBufferToPhysicalIndex(rb, rb->head);
    uint32_t lLinearSpan = rb->capacity - lPhysicalHead;

    return (lFree < lLinearSpan) ? lFree : lLinearSpan;
}

/**
* @brief : Get the largest contiguous read span before wrap-around.
* @param : rb Pointer to the ring buffer control block.
* @return: Number of bytes that can be read linearly.
**/
static uint32_t ringBufferGetLinearReadSpan(const stRingBuffer *rb)
{
    uint32_t lUsed = ringBufferGetUsedInternal(rb);
    uint32_t lPhysicalTail = ringBufferToPhysicalIndex(rb, rb->tail);
    uint32_t lLinearSpan = rb->capacity - lPhysicalTail;

    return (lUsed < lLinearSpan) ? lUsed : lLinearSpan;
}

/**
* @brief : Copy a block of input data into the ring buffer storage.
* @param : rb          Pointer to the ring buffer control block.
* @param : logicalHead Logical start index for the write operation.
* @param : src         Source buffer containing input data.
* @param : length      Number of bytes to copy.
* @return: None
**/
static void ringBufferCopyIn(stRingBuffer *rb, uint32_t logicalHead, const uint8_t *src, uint32_t length)
{
    uint32_t lFirstLength;
    stRingBuffer lSnapshot;

    if (length == 0U) {
        return;
    }

    lSnapshot = *rb;
    lSnapshot.head = logicalHead;
    lFirstLength = ringBufferGetLinearWriteSpan(&lSnapshot);
    if (lFirstLength > length) {
        lFirstLength = length;
    }

    (void)memcpy(&rb->buffer[ringBufferToPhysicalIndex(rb, logicalHead)], src, lFirstLength);

    if (length > lFirstLength) {
        (void)memcpy(rb->buffer, &src[lFirstLength], length - lFirstLength);
    }
}

/**
* @brief : Copy a block of data out of the ring buffer storage.
* @param : rb          Pointer to the ring buffer control block.
* @param : logicalTail Logical start index for the read operation.
* @param : dst         Destination buffer for output data.
* @param : length      Number of bytes to copy.
* @return: None
**/
static void ringBufferCopyOut(const stRingBuffer *rb, uint32_t logicalTail, uint8_t *dst, uint32_t length)
{
    uint32_t lFirstLength;
    stRingBuffer lSnapshot;

    if (length == 0U) {
        return;
    }

    lSnapshot = *rb;
    lSnapshot.tail = logicalTail;
    lFirstLength = ringBufferGetLinearReadSpan(&lSnapshot);
    if (lFirstLength > length) {
        lFirstLength = length;
    }

    (void)memcpy(dst, &rb->buffer[ringBufferToPhysicalIndex(rb, logicalTail)], lFirstLength);

    if (length > lFirstLength) {
        (void)memcpy(&dst[lFirstLength], rb->buffer, length - lFirstLength);
    }
}

/**
* @brief : Initialize a ring buffer instance with caller-provided storage.
* @param : rb       Pointer to the ring buffer control block.
* @param : storage  Pointer to the backing storage buffer.
* @param : capacity Storage capacity in bytes.
* @return: Initialization status code.
**/
eRingBufferStatus ringBufferInit(stRingBuffer *rb, uint8_t *storage, uint32_t capacity)
{
    if ((rb == NULL) || (storage == NULL) || (capacity == 0U) || (capacity > RINGBUFFER_MAX_CAPACITY)) {
        return RINGBUFFER_ERROR_PARAM;
    }

    rb->buffer = storage;
    rb->capacity = capacity;
    rb->head = 0U;
    rb->tail = 0U;
    rb->isPowerOfTwo = ringBufferIsPowerOfTwoInternal(capacity);
    rb->mask = (rb->isPowerOfTwo != 0U) ? (capacity - 1U) : 0U;

    return RINGBUFFER_OK;
}

/**
* @brief : Reset the ring buffer to the empty state.
* @param : rb Pointer to the ring buffer control block.
* @return: Operation status code.
**/
eRingBufferStatus ringBufferReset(stRingBuffer *rb)
{
    if (ringBufferIsConfigured(rb) == 0U) {
        return RINGBUFFER_ERROR_PARAM;
    }

    ringBufferEnterCritical();
    ringBufferMemoryBarrier();
    rb->head = 0U;
    rb->tail = 0U;
    ringBufferMemoryBarrier();
    ringBufferExitCritical();

    return RINGBUFFER_OK;
}

/**
* @brief : Query the number of bytes currently stored in the ring buffer.
* @param : rb Pointer to the ring buffer control block.
* @return: Used byte count, or zero when the state is invalid.
**/
uint32_t ringBufferGetUsed(const stRingBuffer *rb)
{
    if (ringBufferHasValidState(rb) == 0U) {
        return 0U;
    }

    return ringBufferGetUsedInternal(rb);
}

/**
* @brief : Query the remaining free capacity of the ring buffer.
* @param : rb Pointer to the ring buffer control block.
* @return: Free byte count, or zero when the state is invalid.
**/
uint32_t ringBufferGetFree(const stRingBuffer *rb)
{
    if (ringBufferHasValidState(rb) == 0U) {
        return 0U;
    }

    return rb->capacity - ringBufferGetUsedInternal(rb);
}

/**
* @brief : Get the configured storage capacity of the ring buffer.
* @param : rb Pointer to the ring buffer control block.
* @return: Total capacity in bytes.
**/
uint32_t ringBufferGetCapacity(const stRingBuffer *rb)
{
    if (ringBufferIsConfigured(rb) == 0U) {
        return 0U;
    }

    return rb->capacity;
}

/**
* @brief : Check whether the ring buffer currently contains no data.
* @param : rb Pointer to the ring buffer control block.
* @return: Non-zero when the ring buffer is empty, otherwise zero.
**/
uint8_t ringBufferIsEmpty(const stRingBuffer *rb)
{
    if (ringBufferHasValidState(rb) == 0U) {
        return 0U;
    }

    return (uint8_t)(ringBufferGetUsedInternal(rb) == 0U);
}

/**
* @brief : Check whether the ring buffer has no free space left.
* @param : rb Pointer to the ring buffer control block.
* @return: Non-zero when the ring buffer is full, otherwise zero.
**/
uint8_t ringBufferIsFull(const stRingBuffer *rb)
{
    if (ringBufferHasValidState(rb) == 0U) {
        return 0U;
    }

    return (uint8_t)(ringBufferGetUsedInternal(rb) == rb->capacity);
}

/**
* @brief : Push a single byte into the ring buffer.
* @param : rb   Pointer to the ring buffer control block.
* @param : data Byte value to write.
* @return: Operation status code.
**/
eRingBufferStatus ringBufferPushByte(stRingBuffer *rb, uint8_t data)
{
    uint32_t lUsed;
    uint32_t lWriteIndex;

    if (ringBufferIsConfigured(rb) == 0U) {
        return RINGBUFFER_ERROR_PARAM;
    }

    lUsed = ringBufferGetUsedInternal(rb);
    if (lUsed > rb->capacity) {
        return RINGBUFFER_ERROR_STATE;
    }
    if (lUsed == rb->capacity) {
        return RINGBUFFER_ERROR_FULL;
    }

    lWriteIndex = ringBufferToPhysicalIndex(rb, rb->head);
    rb->buffer[lWriteIndex] = data;
    ringBufferMemoryBarrier();
    rb->head++;

    return RINGBUFFER_OK;
}

/**
* @brief : Pop a single byte from the ring buffer.
* @param : rb   Pointer to the ring buffer control block.
* @param : data Output pointer that receives the byte.
* @return: Operation status code.
**/
eRingBufferStatus ringBufferPopByte(stRingBuffer *rb, uint8_t *data)
{
    uint32_t lUsed;
    uint32_t lReadIndex;

    if ((ringBufferIsConfigured(rb) == 0U) || (data == NULL)) {
        return RINGBUFFER_ERROR_PARAM;
    }

    lUsed = ringBufferGetUsedInternal(rb);
    if (lUsed > rb->capacity) {
        return RINGBUFFER_ERROR_STATE;
    }
    if (lUsed == 0U) {
        return RINGBUFFER_ERROR_EMPTY;
    }

    ringBufferMemoryBarrier();
    lReadIndex = ringBufferToPhysicalIndex(rb, rb->tail);
    *data = rb->buffer[lReadIndex];
    ringBufferMemoryBarrier();
    rb->tail++;

    return RINGBUFFER_OK;
}

/**
* @brief : Read a single byte from the ring buffer without consuming it.
* @param : rb   Pointer to the ring buffer control block.
* @param : data Output pointer that receives the byte.
* @return: Operation status code.
**/
eRingBufferStatus ringBufferPeekByte(const stRingBuffer *rb, uint8_t *data)
{
    uint32_t lUsed;
    uint32_t lReadIndex;

    if ((ringBufferIsConfigured(rb) == 0U) || (data == NULL)) {
        return RINGBUFFER_ERROR_PARAM;
    }

    lUsed = ringBufferGetUsedInternal(rb);
    if (lUsed > rb->capacity) {
        return RINGBUFFER_ERROR_STATE;
    }
    if (lUsed == 0U) {
        return RINGBUFFER_ERROR_EMPTY;
    }

    ringBufferMemoryBarrier();
    lReadIndex = ringBufferToPhysicalIndex(rb, rb->tail);
    *data = rb->buffer[lReadIndex];

    return RINGBUFFER_OK;
}

/**
* @brief : Write as many bytes as possible from the source buffer.
* @param : rb     Pointer to the ring buffer control block.
* @param : src    Source buffer containing input data.
* @param : length Requested byte count to write.
* @return: Actual number of bytes written.
**/
uint32_t ringBufferWrite(stRingBuffer *rb, const uint8_t *src, uint32_t length)
{
    uint32_t lFree;
    uint32_t lWriteLength;

    if ((ringBufferIsConfigured(rb) == 0U) || ((src == NULL) && (length != 0U))) {
        return 0U;
    }

    if (length == 0U) {
        return 0U;
    }

    lFree = ringBufferGetFree(rb);
    lWriteLength = (length < lFree) ? length : lFree;
    if (lWriteLength == 0U) {
        return 0U;
    }

    ringBufferCopyIn(rb, rb->head, src, lWriteLength);
    ringBufferMemoryBarrier();
    rb->head += lWriteLength;

    return lWriteLength;
}

/**
* @brief : Read as many bytes as possible into the destination buffer.
* @param : rb     Pointer to the ring buffer control block.
* @param : dst    Destination buffer for output data.
* @param : length Requested byte count to read.
* @return: Actual number of bytes read.
**/
uint32_t ringBufferRead(stRingBuffer *rb, uint8_t *dst, uint32_t length)
{
    uint32_t lUsed;
    uint32_t lReadLength;

    if ((ringBufferIsConfigured(rb) == 0U) || ((dst == NULL) && (length != 0U))) {
        return 0U;
    }

    if (length == 0U) {
        return 0U;
    }

    lUsed = ringBufferGetUsed(rb);
    lReadLength = (length < lUsed) ? length : lUsed;
    if (lReadLength == 0U) {
        return 0U;
    }

    ringBufferMemoryBarrier();
    ringBufferCopyOut(rb, rb->tail, dst, lReadLength);
    ringBufferMemoryBarrier();
    rb->tail += lReadLength;

    return lReadLength;
}

/**
* @brief : Copy buffered data into the destination buffer without consuming it.
* @param : rb     Pointer to the ring buffer control block.
* @param : dst    Destination buffer for output data.
* @param : length Requested byte count to peek.
* @return: Actual number of bytes copied.
**/
uint32_t ringBufferPeek(const stRingBuffer *rb, uint8_t *dst, uint32_t length)
{
    uint32_t lUsed;
    uint32_t lReadLength;

    if ((ringBufferIsConfigured(rb) == 0U) || ((dst == NULL) && (length != 0U))) {
        return 0U;
    }

    if (length == 0U) {
        return 0U;
    }

    lUsed = ringBufferGetUsed(rb);
    lReadLength = (length < lUsed) ? length : lUsed;
    if (lReadLength == 0U) {
        return 0U;
    }

    ringBufferMemoryBarrier();
    ringBufferCopyOut(rb, rb->tail, dst, lReadLength);

    return lReadLength;
}

/**
* @brief : Discard buffered data without copying it out.
* @param : rb     Pointer to the ring buffer control block.
* @param : length Requested byte count to discard.
* @return: Actual number of bytes discarded.
**/
uint32_t ringBufferDiscard(stRingBuffer *rb, uint32_t length)
{
    uint32_t lUsed;
    uint32_t lDiscardLength;

    if (ringBufferIsConfigured(rb) == 0U) {
        return 0U;
    }

    if (length == 0U) {
        return 0U;
    }

    lUsed = ringBufferGetUsed(rb);
    lDiscardLength = (length < lUsed) ? length : lUsed;
    if (lDiscardLength == 0U) {
        return 0U;
    }

    ringBufferMemoryBarrier();
    rb->tail += lDiscardLength;

    return lDiscardLength;
}

/**
* @brief : Write data and overwrite the oldest buffered bytes when space is insufficient.
* @param : rb     Pointer to the ring buffer control block.
* @param : src    Source buffer containing input data.
* @param : length Requested byte count to write.
* @return: Original input length when accepted, otherwise zero.
**/
uint32_t ringBufferWriteOverwrite(stRingBuffer *rb, const uint8_t *src, uint32_t length)
{
    uint32_t lInputLength;
    uint32_t lWriteLength;
    uint32_t lOverflowLength;
    uint32_t lFree;
    uint32_t lNewHead;

    if ((ringBufferIsConfigured(rb) == 0U) || ((src == NULL) && (length != 0U))) {
        return 0U;
    }

    if (length == 0U) {
        return 0U;
    }

    if (ringBufferHasValidState(rb) == 0U) {
        return 0U;
    }

    lInputLength = length;
    lWriteLength = length;
    if (lWriteLength >= rb->capacity) {
        src = &src[lWriteLength - rb->capacity];
        lWriteLength = rb->capacity;
    }

    ringBufferEnterCritical();

    lFree = rb->capacity - ringBufferGetUsedInternal(rb);
    lOverflowLength = (lWriteLength > lFree) ? (lWriteLength - lFree) : 0U;

    if (lOverflowLength != 0U) {
        rb->tail += lOverflowLength;
    }

    ringBufferCopyIn(rb, rb->head, src, lWriteLength);
    ringBufferMemoryBarrier();
    lNewHead = rb->head + lWriteLength;
    rb->head = lNewHead;
    if (ringBufferGetUsedInternal(rb) > rb->capacity) {
        rb->tail = rb->head - rb->capacity;
    }
    ringBufferMemoryBarrier();

    ringBufferExitCritical();

    return lInputLength;
}

/**************************End of file********************************/
