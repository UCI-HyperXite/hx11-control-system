#include "VL6180.h"

// Minimal init sequence from datasheet
HAL_StatusTypeDef VL6180_Init(I2C_HandleTypeDef *hi2c)
{
    HAL_StatusTypeDef ret;
    uint8_t data;

    // Example: read ID to check communication
    ret = HAL_I2C_Mem_Read(hi2c, VL6180_ADDR, 0x000, I2C_MEMADD_SIZE_16BIT, &data, 1, HAL_MAX_DELAY);
    if(ret != HAL_OK) return ret;

    // Recommended init registers (example, add more per datasheet if needed)
    data = 0x01;
    ret = HAL_I2C_Mem_Write(hi2c, VL6180_ADDR, 0x0207, I2C_MEMADD_SIZE_16BIT, &data, 1, HAL_MAX_DELAY);
    if(ret != HAL_OK) return ret;

    data = 0x01;
    ret = HAL_I2C_Mem_Write(hi2c, VL6180_ADDR, 0x0208, I2C_MEMADD_SIZE_16BIT, &data, 1, HAL_MAX_DELAY);
    if(ret != HAL_OK) return ret;

    HAL_Delay(10); // allow sensor to settle

    return HAL_OK;
}

uint8_t VL6180_ReadRange(I2C_HandleTypeDef *hi2c)
{
    uint8_t cmd = 0x01;
    uint8_t value;

    // Start measurement
    HAL_I2C_Mem_Write(hi2c, VL6180_ADDR, 0x018, I2C_MEMADD_SIZE_16BIT, &cmd, 1, HAL_MAX_DELAY);

    HAL_Delay(10); // wait for measurement (~10ms)

    // Read distance result
    HAL_I2C_Mem_Read(hi2c, VL6180_ADDR, 0x062, I2C_MEMADD_SIZE_16BIT, &value, 1, HAL_MAX_DELAY);

    return value;
}
