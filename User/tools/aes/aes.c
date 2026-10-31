/***********************************************************************************
* @file     : aes.c
* @brief    : AES block cipher implementation.
* @details  : Reusable AES-128/192/256 ECB and CBC implementation.
* @author   : GitHub Copilot
* @date     : 2026-04-21
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "aes.h"

#include <string.h>

static const uint8_t gAesSBox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

static const uint8_t gAesInvSBox[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

static void aesRotateWordLeft(uint8_t *word);
static void aesXorBytes(uint8_t *data, const uint8_t *mask, uint8_t length);
static void aesAddRoundKey(uint8_t *data, const uint8_t *roundKey);
static void aesSubstituteBytes(uint8_t *data, uint8_t length, const uint8_t *box);
static void aesShiftRows(uint8_t *state, uint8_t inverse);
static uint8_t aesGfMultBy02(uint8_t value);
static void aesMixColumns(uint8_t *data, uint8_t inverse);
static void aesBlockEncrypt(const stAesContext *context, uint8_t *data);
static void aesBlockDecrypt(const stAesContext *context, uint8_t *data);
static eAesStatus aesResolveType(stAesContext *context, eAesType type);

static void aesRotateWordLeft(uint8_t *word)
{
    uint8_t firstByte;

    firstByte = word[0];
    word[0] = word[1];
    word[1] = word[2];
    word[2] = word[3];
    word[3] = firstByte;
}

static void aesXorBytes(uint8_t *data, const uint8_t *mask, uint8_t length)
{
    uint8_t index;

    for (index = 0; index < length; index++) {
        data[index] ^= mask[index];
    }
}

static void aesAddRoundKey(uint8_t *data, const uint8_t *roundKey)
{
    aesXorBytes(data, roundKey, AES_BLOCK_SIZE);
}

static void aesSubstituteBytes(uint8_t *data, uint8_t length, const uint8_t *box)
{
    uint8_t index;

    for (index = 0; index < length; index++) {
        data[index] = box[data[index]];
    }
}

static void aesShiftRows(uint8_t *state, uint8_t inverse)
{
    uint8_t rowIndex;
    uint8_t columnIndex;
    uint8_t shiftCount;
    uint8_t rowBuffer[4];

    for (rowIndex = 1; rowIndex < 4; rowIndex++) {
        for (columnIndex = 0; columnIndex < 4; columnIndex++) {
            rowBuffer[columnIndex] = state[rowIndex + (4U * columnIndex)];
        }

        shiftCount = inverse ? (uint8_t)(4U - rowIndex) : rowIndex;
        for (columnIndex = 0; columnIndex < 4; columnIndex++) {
            state[rowIndex + (4U * columnIndex)] = rowBuffer[(columnIndex + shiftCount) % 4U];
        }
    }
}

static uint8_t aesGfMultBy02(uint8_t value)
{
    if ((value & 0x80U) == 0U) {
        return (uint8_t)(value << 1);
    }

    return (uint8_t)((value << 1) ^ 0x1BU);
}

static void aesMixColumns(uint8_t *data, uint8_t inverse)
{
    uint8_t columnIndex;
    uint8_t temp;
    uint8_t sum02;
    uint8_t sum13;
    uint8_t result[4];

    for (columnIndex = 0; columnIndex < 4; columnIndex++, data += 4) {
        temp = (uint8_t)(data[0] ^ data[1] ^ data[2] ^ data[3]);
        result[0] = (uint8_t)(temp ^ data[0] ^ aesGfMultBy02((uint8_t)(data[0] ^ data[1])));
        result[1] = (uint8_t)(temp ^ data[1] ^ aesGfMultBy02((uint8_t)(data[1] ^ data[2])));
        result[2] = (uint8_t)(temp ^ data[2] ^ aesGfMultBy02((uint8_t)(data[2] ^ data[3])));
        result[3] = (uint8_t)(temp ^ data[3] ^ aesGfMultBy02((uint8_t)(data[3] ^ data[0])));

        if (inverse != 0U) {
            sum02 = aesGfMultBy02(aesGfMultBy02((uint8_t)(data[0] ^ data[2])));
            sum13 = aesGfMultBy02(aesGfMultBy02((uint8_t)(data[1] ^ data[3])));
            temp = aesGfMultBy02((uint8_t)(sum02 ^ sum13));
            result[0] ^= (uint8_t)(temp ^ sum02);
            result[1] ^= (uint8_t)(temp ^ sum13);
            result[2] ^= (uint8_t)(temp ^ sum02);
            result[3] ^= (uint8_t)(temp ^ sum13);
        }

        memcpy(data, result, sizeof(result));
    }
}

static void aesBlockEncrypt(const stAesContext *context, uint8_t *data)
{
    uint8_t roundIndex;

    aesAddRoundKey(data, context->expandKey);
    for (roundIndex = 1; roundIndex <= context->nr; roundIndex++) {
        aesSubstituteBytes(data, AES_BLOCK_SIZE, gAesSBox);
        aesShiftRows(data, 0U);
        if (roundIndex != context->nr) {
            aesMixColumns(data, 0U);
        }
        aesAddRoundKey(data, &context->expandKey[AES_ROUND_KEY_STEP * roundIndex]);
    }
}

static void aesBlockDecrypt(const stAesContext *context, uint8_t *data)
{
    uint8_t roundIndex;

    aesAddRoundKey(data, &context->expandKey[AES_ROUND_KEY_STEP * context->nr]);
    for (roundIndex = context->nr; roundIndex > 0U; roundIndex--) {
        aesShiftRows(data, 1U);
        aesSubstituteBytes(data, AES_BLOCK_SIZE, gAesInvSBox);
        aesAddRoundKey(data, &context->expandKey[AES_ROUND_KEY_STEP * (roundIndex - 1U)]);
        if (roundIndex != 1U) {
            aesMixColumns(data, 1U);
        }
    }
}

static eAesStatus aesResolveType(stAesContext *context, eAesType type)
{
    context->type = type;
    switch (type) {
        case AES_TYPE_128:
            context->nk = 4U;
            context->nr = 10U;
            return AES_STATUS_OK;
        case AES_TYPE_192:
            context->nk = 6U;
            context->nr = 12U;
            return AES_STATUS_OK;
        case AES_TYPE_256:
            context->nk = 8U;
            context->nr = 14U;
            return AES_STATUS_OK;
        default:
            context->nk = 0U;
            context->nr = 0U;
            return AES_STATUS_ERROR_TYPE;
    }
}

eAesStatus aesInit(stAesContext *context, eAesType type, eAesMode mode, const uint8_t *key, const uint8_t *iv)
{
    uint8_t rcon[4];
    uint8_t *expandKey;
    uint8_t roundWordIndex;
    eAesStatus status;

    if ((context == NULL) || (key == NULL)) {
        return AES_STATUS_ERROR_PARAM;
    }
    if ((mode == AES_MODE_CBC) && (iv == NULL)) {
        return AES_STATUS_ERROR_PARAM;
    }

    memset(context, 0, sizeof(*context));
    status = aesResolveType(context, type);
    if (status != AES_STATUS_OK) {
        return status;
    }

    context->mode = mode;
    if (iv != NULL) {
        memcpy(context->iv, iv, AES_BLOCK_SIZE);
    }

    memcpy(context->expandKey, key, 4U * context->nk);
    expandKey = &context->expandKey[4U * context->nk];
    rcon[0] = 0x01U;
    rcon[1] = 0x00U;
    rcon[2] = 0x00U;
    rcon[3] = 0x00U;

    for (roundWordIndex = context->nk; roundWordIndex < (AES_NB * (context->nr + 1U)); roundWordIndex++, expandKey += 4) {
        memcpy(expandKey, expandKey - 4, 4);
        if ((roundWordIndex % context->nk) == 0U) {
            aesRotateWordLeft(expandKey);
            aesSubstituteBytes(expandKey, 4U, gAesSBox);
            aesXorBytes(expandKey, rcon, 4U);
            rcon[0] = aesGfMultBy02(rcon[0]);
        } else if ((context->nk > 6U) && ((roundWordIndex % context->nk) == AES_NB)) {
            aesSubstituteBytes(expandKey, 4U, gAesSBox);
        }

        aesXorBytes(expandKey, expandKey - (4U * context->nk), 4U);
    }

    return AES_STATUS_OK;
}

eAesStatus aesEncrypt(stAesContext *context, const uint8_t *plainText, uint8_t *cipherText, uint32_t dataLen)
{
    uint32_t offset;
    uint8_t currentIv[AES_BLOCK_SIZE];

    if ((context == NULL) || (plainText == NULL) || (cipherText == NULL)) {
        return AES_STATUS_ERROR_PARAM;
    }
    if ((dataLen % AES_BLOCK_SIZE) != 0U) {
        return AES_STATUS_ERROR_LENGTH;
    }
    if (dataLen == 0U) {
        return AES_STATUS_OK;
    }

    if (plainText != cipherText) {
        memcpy(cipherText, plainText, dataLen);
    }

    if (context->mode == AES_MODE_CBC) {
        memcpy(currentIv, context->iv, sizeof(currentIv));
    }

    for (offset = 0U; offset < dataLen; offset += AES_BLOCK_SIZE) {
        if (context->mode == AES_MODE_CBC) {
            aesXorBytes(&cipherText[offset], currentIv, AES_BLOCK_SIZE);
        }
        aesBlockEncrypt(context, &cipherText[offset]);
        if (context->mode == AES_MODE_CBC) {
            memcpy(currentIv, &cipherText[offset], AES_BLOCK_SIZE);
        }
    }

    return AES_STATUS_OK;
}

eAesStatus aesDecrypt(stAesContext *context, const uint8_t *cipherText, uint8_t *plainText, uint32_t dataLen)
{
    uint32_t offset;
    uint8_t currentIv[AES_BLOCK_SIZE];
    uint8_t cipherBlock[AES_BLOCK_SIZE];

    if ((context == NULL) || (cipherText == NULL) || (plainText == NULL)) {
        return AES_STATUS_ERROR_PARAM;
    }
    if ((dataLen % AES_BLOCK_SIZE) != 0U) {
        return AES_STATUS_ERROR_LENGTH;
    }
    if (dataLen == 0U) {
        return AES_STATUS_OK;
    }

    if (cipherText != plainText) {
        memcpy(plainText, cipherText, dataLen);
    }

    if (context->mode == AES_MODE_CBC) {
        memcpy(currentIv, context->iv, sizeof(currentIv));
    }

    for (offset = 0U; offset < dataLen; offset += AES_BLOCK_SIZE) {
        if (context->mode == AES_MODE_CBC) {
            memcpy(cipherBlock, &plainText[offset], AES_BLOCK_SIZE);
        }
        aesBlockDecrypt(context, &plainText[offset]);
        if (context->mode == AES_MODE_CBC) {
            aesXorBytes(&plainText[offset], currentIv, AES_BLOCK_SIZE);
            memcpy(currentIv, cipherBlock, AES_BLOCK_SIZE);
        }
    }

    return AES_STATUS_OK;
}

uint32_t aesPkcs7Pad(uint8_t *data, uint32_t dataLen, uint32_t bufferCapacity)
{
    uint32_t paddingLength;
    uint32_t newLength;

    if (data == NULL) {
        return 0U;
    }

    paddingLength = AES_BLOCK_SIZE - (dataLen % AES_BLOCK_SIZE);
    if (paddingLength == 0U) {
        paddingLength = AES_BLOCK_SIZE;
    }

    newLength = dataLen + paddingLength;
    if (newLength > bufferCapacity) {
        return 0U;
    }

    memset(&data[dataLen], (int)paddingLength, paddingLength);
    return newLength;
}

uint32_t aesPkcs7Unpad(const uint8_t *data, uint32_t dataLen)
{
    uint32_t paddingLength;
    uint32_t index;

    if ((data == NULL) || (dataLen == 0U) || ((dataLen % AES_BLOCK_SIZE) != 0U)) {
        return 0U;
    }

    paddingLength = data[dataLen - 1U];
    if ((paddingLength == 0U) || (paddingLength > AES_BLOCK_SIZE) || (paddingLength > dataLen)) {
        return 0U;
    }

    for (index = 0U; index < paddingLength; index++) {
        if (data[dataLen - 1U - index] != (uint8_t)paddingLength) {
            return 0U;
        }
    }

    return dataLen - paddingLength;
}

/**************************End of file********************************/
