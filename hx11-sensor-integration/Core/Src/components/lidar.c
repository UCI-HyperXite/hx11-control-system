/*
 * lidar.c
 */

#include "main.h"
#include "cmsis_os.h"
#include "lidar.h"

I2C_HandleTypeDef *LIDAR_I2C_Handler;
static uint32_t m_distance;
uint8_t lidar_cmd = 0x04;

void lidar_init(I2C_HandleTypeDef *hi2c)
{
    LIDAR_I2C_Handler = hi2c;
    uint8_t reset = 0x00;
	HAL_I2C_Mem_Write(hi2c, LIDAR_ADD, 0x00, 1, &reset, 1, 0x1000);
}

void lidar_config(int configur)
{
    // configuration setting for lidar
	lidar_cmd = 0x04;
    HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x00,1,&lidar_cmd,1,0x100);
    switch(configur)
    {
    case 0:
        //default mode , balance mode
    	lidar_cmd=0x80;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x02,1,&lidar_cmd,1,0x1000);
        lidar_cmd=0x04;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x04,1,&lidar_cmd,1,0x1000);
        lidar_cmd=0x00;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x1c,1,&lidar_cmd,1,0x1000);
        break;

    case 1:
        //short range, high speed
    	lidar_cmd=0x1d;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x02,1,&lidar_cmd,1,0x1000);
        lidar_cmd=0x08;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x04,1,&lidar_cmd,1,0x1000);
        lidar_cmd=0x00;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x1c,1,&lidar_cmd,1,0x1000);
        break;

    case 2:
        //default range, higher speed short range
    	lidar_cmd=0x80;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x02,1,&lidar_cmd,1,0x1000);
        lidar_cmd=0x08;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x04,1,&lidar_cmd,1,0x1000);
        lidar_cmd=0x00;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x1c,1,&lidar_cmd,1,0x1000);
        break;

    case 3:
        //maximum Range
    	lidar_cmd=0xff;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x02,1,&lidar_cmd,1,0x1000);
        lidar_cmd=0x08;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x04,1,&lidar_cmd,1,0x1000);
        lidar_cmd=0x00;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x1c,1,&lidar_cmd,1,0x1000);
        break;

    case 4:
        //high sensitivity detection, high  measurement
    	lidar_cmd=0x80;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x02,1,&lidar_cmd,1,0x1000);
        lidar_cmd=0x08;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x04,1,&lidar_cmd,1,0x1000);
        lidar_cmd=0x80;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x1c,1,&lidar_cmd,1,0x1000);
        break;

    case 5:
        //low sensitivity detection , low  measurement
    	lidar_cmd=0x80;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x02,1,&lidar_cmd,1,0x1000);
        lidar_cmd=0x08;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x04,1,&lidar_cmd,1,0x1000);
        lidar_cmd=0xb0;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x1c,1,&lidar_cmd,1,0x1000);
        break;
    }
}

int retrieve_lidar_distance()
{
	uint8_t status = 1;
    uint8_t data[2];

    uint8_t trigger = 0x04;
    if(HAL_I2C_Mem_Write(LIDAR_I2C_Handler, LIDAR_ADD, 0x00, 1, &trigger, 1, 100) != HAL_OK)
    	return -1;

    int timeout = 0;
	while (status & 0x01) {
		HAL_I2C_Mem_Read(LIDAR_I2C_Handler, LIDAR_ADD, 0x01, 1, &status, 1, 100);
		if (++timeout > 50)
			return -2;
		osDelay(1);
	}

    uint8_t reg = 0x8f;
    if(HAL_I2C_Master_Transmit(LIDAR_I2C_Handler, LIDAR_ADD, &reg, 1, 100) != HAL_OK) return -1;
    if(HAL_I2C_Master_Receive(LIDAR_I2C_Handler, LIDAR_ADD, data, 2, 100) != HAL_OK) return -1;

    return (int)((data[0] << 8) | data[1]);
}

int check_lidar_health() {
    uint8_t status;
    if (HAL_I2C_Mem_Read(LIDAR_I2C_Handler, LIDAR_ADD, 0x01, 1, &status, 1, 100) != HAL_OK) {
        return -1;
    }
    return (int)status;
}
