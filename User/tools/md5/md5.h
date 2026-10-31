/************************************************************************************
* @file     : md5.h
* @brief    : MD5 digest public API.
* @details  : Provides reusable MD5 incremental and one-shot helpers.
* @author   : GitHub Copilot
* @date     : 2026-04-21
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef MD5_H
#define MD5_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MD5_DIGEST_SIZE    16U
#define MD5_HEX32_SIZE     33U
#define MD5_HEX16_SIZE     17U

typedef enum eMd5Status {
    MD5_STATUS_OK = 0,
    MD5_STATUS_ERROR_PARAM,
    MD5_STATUS_ERROR_FORMAT
} eMd5Status;

typedef struct stMd5Context {
    uint64_t size;
    uint32_t buffer[4];
    uint8_t input[64];
    uint8_t digest[MD5_DIGEST_SIZE];
} stMd5Context;

void md5Init(stMd5Context *context);
void md5Update(stMd5Context *context, const uint8_t *input, uint32_t inputLen);
void md5Final(stMd5Context *context, uint8_t *digest);

eMd5Status md5CalcString(const char *input, uint8_t *digest);
eMd5Status md5CalcData(const uint8_t *input, uint32_t length, uint8_t *digest);

eMd5Status md5DigestToHex32(const uint8_t *digest, char *output, uint8_t upperCase);
eMd5Status md5DigestToHex16(const uint8_t *digest, char *output, uint8_t upperCase);
eMd5Status md5StringToHex32(const char *input, char *output, uint8_t upperCase);
eMd5Status md5StringToHex16(const char *input, char *output, uint8_t upperCase);
eMd5Status md5HexToDigest(const char *input32, uint8_t *digest);

#ifdef __cplusplus
}
#endif

#endif  // MD5_H
/**************************End of file********************************/
