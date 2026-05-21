/*
 * ws2812b.c
 */


#include "ws2812b.h"

static TIM_HandleTypeDef *ws_tim;
static uint32_t ws_channel;
static volatile uint8_t ws2812_busy = 0;

__attribute__((section(".RAM_D2"), aligned(32)))
static uint16_t pwmData[PWM_BUF_LEN];


static void WS2812_ResetBuffer(void)
{
    for (int i = LED_COUNT * BITS_PER_LED; i < PWM_BUF_LEN; i++)
    {
        pwmData[i] = 0;
    }
}

void WS2812_Init(TIM_HandleTypeDef *htim, uint32_t channel)
{
    ws_tim = htim;
    ws_channel = channel;
    ws2812_busy = 0;
    WS2812_ResetBuffer();
}

void WS2812_SetLED(int led, uint8_t r, uint8_t g, uint8_t b)
{
	if (led < 0 || led >= LED_COUNT)
		return;

    uint32_t color = (g << 16) | (r << 8) | b;

    for (int i = 0; i < 24; i++)
    {
        if (color & (1 << (23 - i)))
            pwmData[led * 24 + i] = WS2812_1;
        else
            pwmData[led * 24 + i] = WS2812_0;
    }
}


void WS2812_SetAll(uint8_t r, uint8_t g, uint8_t b)
{
	WS2812_ResetBuffer();
    for (int i = 0; i < LED_COUNT; i++)
    {
        WS2812_SetLED(i, r, g, b);
    }
}

void WS2812_Start(void)
{
	uint32_t t0 = HAL_GetTick();
	while (ws2812_busy)
	{
	    if (HAL_GetTick() - t0 > 5)   // 5 ms timeout
	    {
	        ws2812_busy = 0;
	        HAL_TIM_PWM_Stop_DMA(ws_tim, ws_channel);
	        break;
	    }
	}
	ws2812_busy = 1;
    HAL_TIM_PWM_Start_DMA(ws_tim, ws_channel, (uint32_t *)pwmData, PWM_BUF_LEN);
}

int WS2812_IsBusy(void) {
	return ws2812_busy;
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == ws_tim)
    {
        HAL_TIM_PWM_Stop_DMA(ws_tim, ws_channel);
        __HAL_TIM_SET_COMPARE(ws_tim, ws_channel, 0);
        ws2812_busy = 0;
    }
}

void solid_color(uint8_t r, uint8_t g, uint8_t b) {
	WS2812_SetAll(r, g, b);
	WS2812_Start();
}

