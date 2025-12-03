#ifndef VL6180_H
#define VL6180_H

#include "stm32h7xx_hal.h"

#define VL6180_ADDR (0x29 << 1) // 7-bit I2C address shifted for HAL

HAL_StatusTypeDef VL6180_Init(I2C_HandleTypeDef *hi2c);
uint8_t VL6180_ReadRange(I2C_HandleTypeDef *hi2c);

#endif
