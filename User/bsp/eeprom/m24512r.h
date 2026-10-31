/************************************************************************************
* @file     : m24512r.h
* @brief    : M24512-R EEPROM driver.
* @details  : Provides byte reads and page-aware writes over software I2C.
* @author   :
* @date     :
* @version  :
* @copyright: Copyright (c) 2050
***********************************************************************************/
#ifndef USER_BSP_EEPROM_M24512R_H
#define USER_BSP_EEPROM_M24512R_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define M24512R_STATUS_OK                    1
#define M24512R_ERROR_INVALID_PARAM        (-1)
#define M24512R_ERROR_NOT_READY            (-2)
#define M24512R_ERROR_NACK                 (-3)
#define M24512R_ERROR_TIMEOUT              (-4)

#define M24512R_CAPACITY_BYTES          65536UL
#define M24512R_PAGE_SIZE                  64U
#define M24512R_DEVICE_ADDRESS_7BIT       0x50U
#define M24512R_SCL_PIN                   (1UL << 10U)
#define M24512R_SDA_PIN                   (1UL << 11U)
#define M24512R_HALF_PERIOD_US            5U
#define M24512R_WRITE_TIMEOUT_US          10000U

int8_t m24512rInit(void);
int8_t m24512rReadBytes(uint16_t address, uint8_t *data, uint16_t length);
int8_t m24512rWriteBytes(uint16_t address, const uint8_t *data, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif /* USER_BSP_EEPROM_M24512R_H */
/**************************End of file********************************/
