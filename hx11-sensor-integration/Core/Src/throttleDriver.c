#include "throttleDriver.h"

//resources used: https://wiki.st.com/stm32mcu/wiki/Getting_started_with_DAC
void throttleTest() {
	//start throttle at 0V, jump in increments of 1.22mV to 5V.
	for (int dac_value = 0; dac_value <= DAC_MAX_VALUE; dac_value++) {
		//sets DAC_CHANNEL_2 to voltage at dac_value (built-in DAC converts
		//channel_2 outputting on PA5
		HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac_value);
		if (dac_value < 4095) { //255 if using 8-bit
			dac_value++;
		} else {
			dac_value = 0;
		}
		HAL_Delay(1);
	}

	//decrement test
	for (int dac_value = DAC_MAX_VALUE; dac_value >= 0; dac_value--) {
		HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac_value); //do we need 8-bits or 12-bits?
		if (dac_value > 0) { //255 if using 8-bit
			dac_value--;
		} else {
			dac_value = DAC_MAX_VALUE;
		}
		HAL_Delay(1);
	}
}

void accelerate(int input) {
	for (int dac_value = 0; dac_value <= input; dac_value++) {
		//sets DAC_CHANNEL_2 to voltage at dac_value (built-in DAC converts
		//channel_2 outputting on PA5
		HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac_value);
		HAL_Delay(1); //change if its too fast (the LIM blows up)
	}
}

void killThrottle() {
	HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, 0);
}
