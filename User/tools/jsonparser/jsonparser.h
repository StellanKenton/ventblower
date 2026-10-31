/************************************************************************************
* @file     : jsonparser.h
* @brief    : Lightweight JSON field parser.
* @details  : Provides allocation-free helpers for extracting simple JSON values.
* @author   : GitHub Copilot
* @date     : 2026-04-25
* @version  : V1.0.0
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef JSONPARSER_H
#define JSONPARSER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum eJsonParserStatus {
    JSON_PARSER_STATUS_OK = 0,
    JSON_PARSER_STATUS_ERROR_PARAM,
    JSON_PARSER_STATUS_NOT_FOUND,
    JSON_PARSER_STATUS_ERROR_FORMAT,
    JSON_PARSER_STATUS_ERROR_BUFFER,
} eJsonParserStatus;

eJsonParserStatus jsonParserFindString(const char *json,
                                       uint16_t jsonLen,
                                       const char *key,
                                       char *output,
                                       uint16_t outputSize,
                                       uint16_t *actualLen);
eJsonParserStatus jsonParserFindInt(const char *json, uint16_t jsonLen, const char *key, int32_t *value);

#ifdef __cplusplus
}
#endif

#endif  // JSONPARSER_H
/**************************End of file********************************/
