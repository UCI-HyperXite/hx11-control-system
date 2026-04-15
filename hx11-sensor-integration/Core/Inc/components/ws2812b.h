/*
 * ws2812b.h
 */

#ifndef INC_WS2812B_H_
#define INC_WS2812B_H_

#include "stm32h7xx_hal.h"

#define LED_COUNT      150
#define BITS_PER_LED   24
// prescaler = 0, timer clock = 64MHz, ARR = 80-1, pwm freq = 800 kHz
#define WS2812_0       26   // ~0.4
#define WS2812_1       52   // ~0.8

#define RESET_SLOTS    50
#define PWM_BUF_LEN    (LED_COUNT * BITS_PER_LED + RESET_SLOTS)

void WS2812_Init(TIM_HandleTypeDef *htim, uint32_t channel);
void WS2812_SetLED(int led, uint8_t r, uint8_t g, uint8_t b);
void WS2812_SetAll(uint8_t r, uint8_t g, uint8_t b);
void WS2812_Start(void);
int WS2812_IsBusy(void);

#endif /* INC_WS2812B_H_ */
