/*
 * peripherals.h
 *
 */

#ifndef INC_HAL_PERIPHERALS_H_
#define INC_HAL_PERIPHERALS_H_

#include "main.h"

extern COM_InitTypeDef BspCOMInit;
extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;
extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim1;
extern DMA_HandleTypeDef hdma_tim1_ch1;
extern UART_HandleTypeDef huart7;

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_DMA_Init(void);
void MX_I2C1_Init(void);
void MX_ADC1_Init(void);
void MX_TIM1_Init(void);
void MX_UART7_Init(void);

#endif /* INC_HAL_PERIPHERALS_H_ */
