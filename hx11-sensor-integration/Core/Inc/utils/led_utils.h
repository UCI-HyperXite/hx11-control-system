/*
 * led_utils.h
 */

#ifndef LED_UTILS_H
#define LED_UTILS_H

#include <stdint.h>
#include "cmsis_os.h"
#include "ws2812b.h"

/**
 * @brief Blink all WS2812 LEDs yellow
 */
void blink_yellow(void);

/**
 * @brief Blink all WS2812 LEDs red
 */
void blink_red(void);

/**
 * @brief Blink all WS2812 LEDs with an arbitrary colour
 *
 * @param r  Red   channel (0-255)
 * @param g  Green channel (0-255)
 * @param b  Blue  channel (0-255)
 */
void blink_color(uint8_t r, uint8_t g, uint8_t b);

#endif /* LED_UTILS_H */
