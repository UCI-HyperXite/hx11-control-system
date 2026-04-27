/*
 * lidar.c
 */

#include "main.h"
#include "cmsis_os.h"
#include "lidar.h"

I2C_HandleTypeDef *LIDAR_I2C_Handler;
static uint32_t m_distance;
static uint8_t cmd[1];
static uint8_t data[2]={10};

void lidar_init(I2C_HandleTypeDef *hi2c)
{
    LIDAR_I2C_Handler = hi2c;
}

void lidar_config(int configur)
{
    // configuration setting for lidar
    cmd[0] = 0x04;
    HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x00,1,cmd,1,0x100);
    switch(configur)
    {
    case 0:
        //default mode , balance mode
        cmd[0]=0x80;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x02,1,cmd,1,0x1000);
        cmd[0]=0x04;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x04,1,cmd,1,0x1000);
        cmd[0]=0x00;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x1c,1,cmd,1,0x1000);
        break;

    case 1:
        //short range, high speed
        cmd[0]=0x1d;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x02,1,cmd,1,0x1000);
        cmd[0]=0x08;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x04,1,cmd,1,0x1000);
        cmd[0]=0x00;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x1c,1,cmd,1,0x1000);
        break;

    case 2:
        //default range, higher speed short range
        cmd[0]=0x80;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x02,1,cmd,1,0x1000);
        cmd[0]=0x08;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x04,1,cmd,1,0x1000);
        cmd[0]=0x00;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x1c,1,cmd,1,0x1000);
        break;

    case 3:
        //maximum Range
        cmd[0]=0xff;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x02,1,cmd,1,0x1000);
        cmd[0]=0x08;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x04,1,cmd,1,0x1000);
        cmd[0]=0x00;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x1c,1,cmd,1,0x1000);
        break;

    case 4:
        //high sensitivity detection, high  measurement
        cmd[0]=0x80;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x02,1,cmd,1,0x1000);
        cmd[0]=0x08;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x04,1,cmd,1,0x1000);
        cmd[0]=0x80;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x1c,1,cmd,1,0x1000);
        break;

    case 5:
        //low sensitivity detection , low  measurement
        cmd[0]=0x80;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x02,1,cmd,1,0x1000);
        cmd[0]=0x08;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x04,1,cmd,1,0x1000);
        cmd[0]=0xb0;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x1c,1,cmd,1,0x1000);
        break;
    }
}

int retrieve_lidar_distance()
{
    cmd[0]=0x04;
    HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x00,1,cmd,1,100);
    cmd[0]=0x8f;
    HAL_I2C_Master_Transmit(LIDAR_I2C_Handler,LIDAR_ADD,cmd,1,100);
    HAL_I2C_Master_Receive(LIDAR_I2C_Handler,LIDAR_ADD,data,2,100);
    m_distance = (data[0]<<8)|(data[1]);
    return m_distance ;
}
