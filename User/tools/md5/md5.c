/***********************************************************************************
* @file     : md5.c
* @brief    : MD5 digest implementation.
* @details  : Reusable MD5 incremental and one-shot helpers.
* @author   : GitHub Copilot
* @date     : 2026-04-21
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "md5.h"

#include <string.h>

#define MD5_A    (0x67452301UL)
#define MD5_B    (0xefcdab89UL)
#define MD5_C    (0x98badcfeUL)
#define MD5_D    (0x10325476UL)

#define MD5_F(x, y, z)  (((x) & (y)) | (~(x) & (z)))
#define MD5_G(x, y, z)  (((x) & (z)) | ((y) & ~(z)))
#define MD5_FUNC_H(x, y, z)  ((x) ^ (y) ^ (z))
#define MD5_I(x, y, z)  ((y) ^ ((x) | ~(z)))

static const uint32_t gMd5Shift[64] = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
};

static const uint32_t gMd5Table[64] = {
    0xd76aa478UL, 0xe8c7b756UL, 0x242070dbUL, 0xc1bdceeeUL,
    0xf57c0fafUL, 0x4787c62aUL, 0xa8304613UL, 0xfd469501UL,
    0x698098d8UL, 0x8b44f7afUL, 0xffff5bb1UL, 0x895cd7beUL,
    0x6b901122UL, 0xfd987193UL, 0xa679438eUL, 0x49b40821UL,
    0xf61e2562UL, 0xc040b340UL, 0x265e5a51UL, 0xe9b6c7aaUL,
    0xd62f105dUL, 0x02441453UL, 0xd8a1e681UL, 0xe7d3fbc8UL,
    0x21e1cde6UL, 0xc33707d6UL, 0xf4d50d87UL, 0x455a14edUL,
    0xa9e3e905UL, 0xfcefa3f8UL, 0x676f02d9UL, 0x8d2a4c8aUL,
    0xfffa3942UL, 0x8771f681UL, 0x6d9d6122UL, 0xfde5380cUL,
    0xa4beea44UL, 0x4bdecfa9UL, 0xf6bb4b60UL, 0xbebfbc70UL,
    0x289b7ec6UL, 0xeaa127faUL, 0xd4ef3085UL, 0x04881d05UL,
    0xd9d4d039UL, 0xe6db99e5UL, 0x1fa27cf8UL, 0xc4ac5665UL,
    0xf4292244UL, 0x432aff97UL, 0xab9423a7UL, 0xfc93a039UL,
    0x655b59c3UL, 0x8f0ccc92UL, 0xffeff47dUL, 0x85845dd1UL,
    0x6fa87e4fUL, 0xfe2ce6e0UL, 0xa3014314UL, 0x4e0811a1UL,
    0xf7537e82UL, 0xbd3af235UL, 0x2ad7d2bbUL, 0xeb86d391UL
};

static const uint8_t gMd5Padding[64] = {
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static uint32_t md5RotateLeft(uint32_t value, uint32_t shift);
static void md5Step(uint32_t *buffer, const uint32_t *input);
static char md5NibbleToHex(uint8_t nibble, uint8_t upperCase);
static int8_t md5HexToNibble(char value);

static uint32_t md5RotateLeft(uint32_t value, uint32_t shift)
{
    return (value << shift) | (value >> (32U - shift));
}

static void md5Step(uint32_t *buffer, const uint32_t *input)
{
    uint32_t aa;
    uint32_t bb;
    uint32_t cc;
    uint32_t dd;
    uint32_t roundValue;
    uint32_t inputIndex;
    uint32_t stepIndex;
    uint32_t temp;

    aa = buffer[0];
    bb = buffer[1];
    cc = buffer[2];
    dd = buffer[3];

    for (stepIndex = 0U; stepIndex < 64U; stepIndex++) {
        if (stepIndex < 16U) {
            roundValue = MD5_F(bb, cc, dd);
            inputIndex = stepIndex;
        } else if (stepIndex < 32U) {
            roundValue = MD5_G(bb, cc, dd);
            inputIndex = ((stepIndex * 5U) + 1U) % 16U;
        } else if (stepIndex < 48U) {
            roundValue = MD5_FUNC_H(bb, cc, dd);
            inputIndex = ((stepIndex * 3U) + 5U) % 16U;
        } else {
            roundValue = MD5_I(bb, cc, dd);
            inputIndex = (stepIndex * 7U) % 16U;
        }

        temp = dd;
        dd = cc;
        cc = bb;
        bb = bb + md5RotateLeft(aa + roundValue + gMd5Table[stepIndex] + input[inputIndex], gMd5Shift[stepIndex]);
        aa = temp;
    }

    buffer[0] += aa;
    buffer[1] += bb;
    buffer[2] += cc;
    buffer[3] += dd;
}

static char md5NibbleToHex(uint8_t nibble, uint8_t upperCase)
{
    if (nibble < 10U) {
        return (char)('0' + nibble);
    }

    return (char)((upperCase != 0U ? 'A' : 'a') + (nibble - 10U));
}

static int8_t md5HexToNibble(char value)
{
    if ((value >= '0') && (value <= '9')) {
        return (int8_t)(value - '0');
    }
    if ((value >= 'a') && (value <= 'f')) {
        return (int8_t)(value - 'a' + 10);
    }
    if ((value >= 'A') && (value <= 'F')) {
        return (int8_t)(value - 'A' + 10);
    }

    return -1;
}

void md5Init(stMd5Context *context)
{
    if (context == NULL) {
        return;
    }

    memset(context, 0, sizeof(*context));
    context->buffer[0] = MD5_A;
    context->buffer[1] = MD5_B;
    context->buffer[2] = MD5_C;
    context->buffer[3] = MD5_D;
}

void md5Update(stMd5Context *context, const uint8_t *input, uint32_t inputLen)
{
    uint32_t block[16];
    uint32_t offset;
    uint32_t inputIndex;
    uint32_t wordIndex;

    if ((context == NULL) || ((input == NULL) && (inputLen != 0U))) {
        return;
    }

    offset = (uint32_t)(context->size % 64U);
    context->size += (uint64_t)inputLen;

    for (inputIndex = 0U; inputIndex < inputLen; inputIndex++) {
        context->input[offset++] = input[inputIndex];
        if ((offset % 64U) == 0U) {
            for (wordIndex = 0U; wordIndex < 16U; wordIndex++) {
                block[wordIndex] = ((uint32_t)context->input[(wordIndex * 4U) + 3U] << 24)
                    | ((uint32_t)context->input[(wordIndex * 4U) + 2U] << 16)
                    | ((uint32_t)context->input[(wordIndex * 4U) + 1U] << 8)
                    | ((uint32_t)context->input[(wordIndex * 4U)]);
            }
            md5Step(context->buffer, block);
            offset = 0U;
        }
    }
}

void md5Final(stMd5Context *context, uint8_t *digest)
{
    uint32_t block[16];
    uint32_t offset;
    uint32_t paddingLength;
    uint32_t wordIndex;
    uint32_t digestIndex;

    if ((context == NULL) || (digest == NULL)) {
        return;
    }

    offset = (uint32_t)(context->size % 64U);
    paddingLength = (offset < 56U) ? (56U - offset) : (120U - offset);
    md5Update(context, gMd5Padding, paddingLength);
    context->size -= (uint64_t)paddingLength;

    for (wordIndex = 0U; wordIndex < 14U; wordIndex++) {
        block[wordIndex] = ((uint32_t)context->input[(wordIndex * 4U) + 3U] << 24)
            | ((uint32_t)context->input[(wordIndex * 4U) + 2U] << 16)
            | ((uint32_t)context->input[(wordIndex * 4U) + 1U] << 8)
            | ((uint32_t)context->input[(wordIndex * 4U)]);
    }
    block[14] = (uint32_t)(context->size * 8U);
    block[15] = (uint32_t)((context->size * 8U) >> 32);

    md5Step(context->buffer, block);

    for (digestIndex = 0U; digestIndex < 4U; digestIndex++) {
        context->digest[(digestIndex * 4U) + 0U] = (uint8_t)(context->buffer[digestIndex] & 0x000000FFUL);
        context->digest[(digestIndex * 4U) + 1U] = (uint8_t)((context->buffer[digestIndex] & 0x0000FF00UL) >> 8);
        context->digest[(digestIndex * 4U) + 2U] = (uint8_t)((context->buffer[digestIndex] & 0x00FF0000UL) >> 16);
        context->digest[(digestIndex * 4U) + 3U] = (uint8_t)((context->buffer[digestIndex] & 0xFF000000UL) >> 24);
    }

    memcpy(digest, context->digest, MD5_DIGEST_SIZE);
}

eMd5Status md5CalcString(const char *input, uint8_t *digest)
{
    stMd5Context context;

    if ((input == NULL) || (digest == NULL)) {
        return MD5_STATUS_ERROR_PARAM;
    }

    md5Init(&context);
    md5Update(&context, (const uint8_t *)input, (uint32_t)strlen(input));
    md5Final(&context, digest);
    return MD5_STATUS_OK;
}

eMd5Status md5CalcData(const uint8_t *input, uint32_t length, uint8_t *digest)
{
    stMd5Context context;

    if ((digest == NULL) || ((input == NULL) && (length != 0U))) {
        return MD5_STATUS_ERROR_PARAM;
    }

    md5Init(&context);
    md5Update(&context, input, length);
    md5Final(&context, digest);
    return MD5_STATUS_OK;
}

eMd5Status md5DigestToHex32(const uint8_t *digest, char *output, uint8_t upperCase)
{
    uint32_t index;

    if ((digest == NULL) || (output == NULL)) {
        return MD5_STATUS_ERROR_PARAM;
    }

    for (index = 0U; index < MD5_DIGEST_SIZE; index++) {
        output[index * 2U] = md5NibbleToHex((uint8_t)(digest[index] >> 4), upperCase);
        output[(index * 2U) + 1U] = md5NibbleToHex((uint8_t)(digest[index] & 0x0FU), upperCase);
    }
    output[MD5_HEX32_SIZE - 1U] = '\0';
    return MD5_STATUS_OK;
}

eMd5Status md5DigestToHex16(const uint8_t *digest, char *output, uint8_t upperCase)
{
    uint32_t index;
    uint32_t outputIndex;

    if ((digest == NULL) || (output == NULL)) {
        return MD5_STATUS_ERROR_PARAM;
    }

    outputIndex = 0U;
    for (index = 4U; index < 12U; index++) {
        output[outputIndex++] = md5NibbleToHex((uint8_t)(digest[index] >> 4), upperCase);
        output[outputIndex++] = md5NibbleToHex((uint8_t)(digest[index] & 0x0FU), upperCase);
    }
    output[MD5_HEX16_SIZE - 1U] = '\0';
    return MD5_STATUS_OK;
}

eMd5Status md5StringToHex32(const char *input, char *output, uint8_t upperCase)
{
    uint8_t digest[MD5_DIGEST_SIZE];
    eMd5Status status;

    status = md5CalcString(input, digest);
    if (status != MD5_STATUS_OK) {
        return status;
    }

    return md5DigestToHex32(digest, output, upperCase);
}

eMd5Status md5StringToHex16(const char *input, char *output, uint8_t upperCase)
{
    uint8_t digest[MD5_DIGEST_SIZE];
    eMd5Status status;

    status = md5CalcString(input, digest);
    if (status != MD5_STATUS_OK) {
        return status;
    }

    return md5DigestToHex16(digest, output, upperCase);
}

eMd5Status md5HexToDigest(const char *input32, uint8_t *digest)
{
    uint32_t index;
    int8_t highNibble;
    int8_t lowNibble;

    if ((input32 == NULL) || (digest == NULL)) {
        return MD5_STATUS_ERROR_PARAM;
    }
    if (strlen(input32) != (MD5_HEX32_SIZE - 1U)) {
        return MD5_STATUS_ERROR_FORMAT;
    }

    for (index = 0U; index < MD5_DIGEST_SIZE; index++) {
        highNibble = md5HexToNibble(input32[index * 2U]);
        lowNibble = md5HexToNibble(input32[(index * 2U) + 1U]);
        if ((highNibble < 0) || (lowNibble < 0)) {
            return MD5_STATUS_ERROR_FORMAT;
        }
        digest[index] = (uint8_t)(((uint8_t)highNibble << 4) | (uint8_t)lowNibble);
    }

    return MD5_STATUS_OK;
}

/**************************End of file********************************/
