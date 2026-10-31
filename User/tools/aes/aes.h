/************************************************************************************
* @file     : aes.h
* @brief    : AES block cipher public API.
* @details  : Provides reusable AES-128/192/256 ECB and CBC helpers.
* @author   : GitHub Copilot
* @date     : 2026-04-21
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef AES_H
#define AES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AES_BLOCK_SIZE             16U
#define AES_MAX_EXPAND_KEY_SIZE    240U
#define AES_NB                     4U
#define AES_ROUND_KEY_STEP         AES_BLOCK_SIZE

typedef enum eAesType {
    AES_TYPE_128 = 128,
    AES_TYPE_192 = 192,
    AES_TYPE_256 = 256
} eAesType;

typedef enum eAesMode {
    AES_MODE_ECB = 0,
    AES_MODE_CBC = 1
} eAesMode;

typedef enum eAesStatus {
    AES_STATUS_OK = 0,
    AES_STATUS_ERROR_PARAM,
    AES_STATUS_ERROR_TYPE,
    AES_STATUS_ERROR_LENGTH
} eAesStatus;

typedef struct stAesContext {
    uint8_t expandKey[AES_MAX_EXPAND_KEY_SIZE];
    uint8_t iv[AES_BLOCK_SIZE];
    uint8_t nk;
    uint8_t nr;
    eAesType type;
    eAesMode mode;
} stAesContext;

eAesStatus aesInit(stAesContext *context, eAesType type, eAesMode mode, const uint8_t *key, const uint8_t *iv);
eAesStatus aesEncrypt(stAesContext *context, const uint8_t *plainText, uint8_t *cipherText, uint32_t dataLen);
eAesStatus aesDecrypt(stAesContext *context, const uint8_t *cipherText, uint8_t *plainText, uint32_t dataLen);

uint32_t aesPkcs7Pad(uint8_t *data, uint32_t dataLen, uint32_t bufferCapacity);
uint32_t aesPkcs7Unpad(const uint8_t *data, uint32_t dataLen);

#ifdef __cplusplus
}
#endif

#endif  // AES_H
/**************************End of file********************************/
