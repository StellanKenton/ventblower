/***********************************************************************************
* @file     : jsonparser.c
* @brief    : Lightweight JSON field parser implementation.
* @details  : Provides allocation-free helpers for extracting simple JSON values.
* @author   : GitHub Copilot
* @date     : 2026-04-25
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
**********************************************************************************/
#include "jsonparser.h"

#include <stdbool.h>
#include <string.h>

static bool jsonParserIsSpace(char value);
static uint16_t jsonParserSkipSpace(const char *json, uint16_t jsonLen, uint16_t index);
static eJsonParserStatus jsonParserSkipString(const char *json, uint16_t jsonLen, uint16_t index, uint16_t *nextIndex);
static eJsonParserStatus jsonParserReadString(const char *json,
                                              uint16_t jsonLen,
                                              uint16_t index,
                                              char *output,
                                              uint16_t outputSize,
                                              uint16_t *actualLen,
                                              uint16_t *nextIndex);
static eJsonParserStatus jsonParserFindValue(const char *json, uint16_t jsonLen, const char *key, uint16_t *valueIndex);

static bool jsonParserIsSpace(char value)
{
    return (value == ' ') || (value == '\t') || (value == '\r') || (value == '\n');
}

static uint16_t jsonParserSkipSpace(const char *json, uint16_t jsonLen, uint16_t index)
{
    while ((index < jsonLen) && jsonParserIsSpace(json[index])) {
        index++;
    }

    return index;
}

static eJsonParserStatus jsonParserSkipString(const char *json, uint16_t jsonLen, uint16_t index, uint16_t *nextIndex)
{
    bool escaped;

    if ((json == NULL) || (nextIndex == NULL) || (index >= jsonLen) || (json[index] != '"')) {
        return JSON_PARSER_STATUS_ERROR_FORMAT;
    }

    escaped = false;
    index++;
    while (index < jsonLen) {
        if (escaped) {
            escaped = false;
        } else if (json[index] == '\\') {
            escaped = true;
        } else if (json[index] == '"') {
            *nextIndex = (uint16_t)(index + 1U);
            return JSON_PARSER_STATUS_OK;
        }
        index++;
    }

    return JSON_PARSER_STATUS_ERROR_FORMAT;
}

static eJsonParserStatus jsonParserReadString(const char *json,
                                              uint16_t jsonLen,
                                              uint16_t index,
                                              char *output,
                                              uint16_t outputSize,
                                              uint16_t *actualLen,
                                              uint16_t *nextIndex)
{
    uint16_t outLen;
    char value;

    if ((json == NULL) || (output == NULL) || (outputSize == 0U) || (nextIndex == NULL) ||
        (index >= jsonLen) || (json[index] != '"')) {
        return JSON_PARSER_STATUS_ERROR_PARAM;
    }

    outLen = 0U;
    index++;
    while (index < jsonLen) {
        value = json[index++];
        if (value == '"') {
            output[outLen] = '\0';
            if (actualLen != NULL) {
                *actualLen = outLen;
            }
            *nextIndex = index;
            return JSON_PARSER_STATUS_OK;
        }

        if (value == '\\') {
            if (index >= jsonLen) {
                return JSON_PARSER_STATUS_ERROR_FORMAT;
            }
            value = json[index++];
            switch (value) {
                case '"':
                case '\\':
                case '/':
                    break;
                case 'b':
                    value = '\b';
                    break;
                case 'f':
                    value = '\f';
                    break;
                case 'n':
                    value = '\n';
                    break;
                case 'r':
                    value = '\r';
                    break;
                case 't':
                    value = '\t';
                    break;
                default:
                    return JSON_PARSER_STATUS_ERROR_FORMAT;
            }
        }

        if (outLen >= (uint16_t)(outputSize - 1U)) {
            return JSON_PARSER_STATUS_ERROR_BUFFER;
        }
        output[outLen++] = value;
    }

    return JSON_PARSER_STATUS_ERROR_FORMAT;
}

static eJsonParserStatus jsonParserFindValue(const char *json, uint16_t jsonLen, const char *key, uint16_t *valueIndex)
{
    uint16_t index;
    uint16_t nextIndex;
    uint16_t keyLen;

    if ((json == NULL) || (key == NULL) || (valueIndex == NULL)) {
        return JSON_PARSER_STATUS_ERROR_PARAM;
    }

    keyLen = (uint16_t)strlen(key);
    if ((jsonLen == 0U) || (keyLen == 0U)) {
        return JSON_PARSER_STATUS_ERROR_PARAM;
    }

    index = 0U;
    while (index < jsonLen) {
        if (json[index] != '"') {
            index++;
            continue;
        }

        if (((uint16_t)(index + keyLen + 1U) < jsonLen) &&
            (memcmp(&json[index + 1U], key, keyLen) == 0) &&
            (json[index + 1U + keyLen] == '"')) {
            nextIndex = jsonParserSkipSpace(json, jsonLen, (uint16_t)(index + keyLen + 2U));
            if ((nextIndex < jsonLen) && (json[nextIndex] == ':')) {
                *valueIndex = jsonParserSkipSpace(json, jsonLen, (uint16_t)(nextIndex + 1U));
                return JSON_PARSER_STATUS_OK;
            }
        }

        if (jsonParserSkipString(json, jsonLen, index, &nextIndex) != JSON_PARSER_STATUS_OK) {
            return JSON_PARSER_STATUS_ERROR_FORMAT;
        }
        index = nextIndex;
    }

    return JSON_PARSER_STATUS_NOT_FOUND;
}

eJsonParserStatus jsonParserFindString(const char *json,
                                       uint16_t jsonLen,
                                       const char *key,
                                       char *output,
                                       uint16_t outputSize,
                                       uint16_t *actualLen)
{
    uint16_t valueIndex;
    uint16_t nextIndex;
    eJsonParserStatus status;

    if ((output == NULL) || (outputSize == 0U)) {
        return JSON_PARSER_STATUS_ERROR_PARAM;
    }

    status = jsonParserFindValue(json, jsonLen, key, &valueIndex);
    if (status != JSON_PARSER_STATUS_OK) {
        return status;
    }

    if ((valueIndex >= jsonLen) || (json[valueIndex] != '"')) {
        return JSON_PARSER_STATUS_ERROR_FORMAT;
    }

    return jsonParserReadString(json, jsonLen, valueIndex, output, outputSize, actualLen, &nextIndex);
}

eJsonParserStatus jsonParserFindInt(const char *json, uint16_t jsonLen, const char *key, int32_t *value)
{
    uint16_t valueIndex;
    int32_t parsed;
    int32_t sign;
    bool hasDigit;
    eJsonParserStatus status;

    if (value == NULL) {
        return JSON_PARSER_STATUS_ERROR_PARAM;
    }

    status = jsonParserFindValue(json, jsonLen, key, &valueIndex);
    if (status != JSON_PARSER_STATUS_OK) {
        return status;
    }

    if (valueIndex >= jsonLen) {
        return JSON_PARSER_STATUS_ERROR_FORMAT;
    }

    sign = 1;
    if (json[valueIndex] == '-') {
        sign = -1;
        valueIndex++;
    }

    parsed = 0;
    hasDigit = false;
    while ((valueIndex < jsonLen) && (json[valueIndex] >= '0') && (json[valueIndex] <= '9')) {
        hasDigit = true;
        parsed = (parsed * 10) + (int32_t)(json[valueIndex] - '0');
        valueIndex++;
    }

    if (!hasDigit) {
        return JSON_PARSER_STATUS_ERROR_FORMAT;
    }

    *value = parsed * sign;
    return JSON_PARSER_STATUS_OK;
}

/**************************End of file********************************/
