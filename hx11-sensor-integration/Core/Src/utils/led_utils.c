/*
 * led_utils.c
 *
 */

#include "led_utils.h"

void blink_yellow(void)
{
    static uint32_t lastToggle = 0;
    static uint8_t ledOn = 0;

    if (osKernelGetTickCount() - lastToggle > 500)
    {
        lastToggle = osKernelGetTickCount();
        ledOn = !ledOn;

        if (ledOn)
            WS2812_SetAll(128, 128, 0);
        else
            WS2812_SetAll(0, 0, 0);

        WS2812_Start();
    }
}

void blink_red(void)
{
    static uint32_t lastToggle = 0;
    static uint8_t ledOn = 0;

    if (osKernelGetTickCount() - lastToggle > 500)
    {
        lastToggle = osKernelGetTickCount();
        ledOn = !ledOn;

        if (ledOn)
            WS2812_SetAll(180, 0, 0);
        else
            WS2812_SetAll(0, 0, 0);

        WS2812_Start();
    }
}

void blink_color(uint8_t r, uint8_t g, uint8_t b) {
	static uint32_t lastToggle = 0;
	static uint8_t ledOn = 0;
	if (osKernelGetTickCount() - lastToggle > 500) {
		lastToggle = osKernelGetTickCount();
		ledOn = !ledOn;
		if (ledOn) {
			WS2812_SetAll(r, g, b);
		} else {
			WS2812_SetAll(0, 0, 0);
		}
		WS2812_Start();
	}
}

