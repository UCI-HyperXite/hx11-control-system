#ifndef THROTTLE
#define THROTTLE


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "stm32h7xx_hal.h"

#define DAC_MAX_VALUE 4095 //granularity = 12 bits, 4096 increments (1.22mV per change)

extern DAC_HandleTypeDef hdac1; //defined in main

void throttleTest();

void accelerate(int input);

void killThrottle();

#endif
