#include "throttleDriver.h"

//resources used: https://wiki.st.com/stm32mcu/wiki/Getting_started_with_DAC
uint8_t dacBuffer[2];
int count = 0;
int sixseven;

uint8_t rxBuf[2];

// Step 1: Transmit

// Step 2: In TX callback, trigger a read
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
    HAL_I2C_Master_Receive_IT(hi2c, 0xC6, rxBuf, 2);
}

// Step 3: RX callback fires when response is received
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
}

void DAC_SetValue(uint16_t val) {
	//add some sort of RTOS check so buffer isn't overwritten by another call?
	// Clamp to 12-bit max
	if (val > 4095) val = 4095;

	dacBuffer[0] = (uint8_t)((val >> 8) & 0x0F);
	//        C2=0,C1=0  PD1=0,PD0=0   upper 4 bits of value
	dacBuffer[1] = (uint8_t)(val & 0x00FF);
	//        lower 8 bits of value

	HAL_I2C_Master_Transmit_IT(&hi2c1, 0xC6, dacBuffer, 2); // 0x63 << 1 = 0xC6
	sixseven = 67;
}

void throttleTest() {
	//start throttle at 0V, jump in increments of 1.22mV to 5V.
	for (uint16_t dac_value = 0; dac_value <= DAC_MAX_VALUE; dac_value++) { //for loop gets overwritten
		//sets DAC_CHANNEL_2 to voltage at dac_value (built-in DAC converts
		//channel_2 outputting on PA5
		//HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac_value);
		DAC_SetValue(dac_value);
		if (dac_value < DAC_MAX_VALUE) { //255 if using 8-bit
			dac_value++;
		} else {
			dac_value = 0;
		}
		HAL_Delay(2);
	}

	//decrement test
	for (uint16_t dac_value = DAC_MAX_VALUE; dac_value >= 0; dac_value--) {
		//HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac_value); //do we need 8-bits or 12-bits?
		DAC_SetValue(dac_value);
		if (dac_value > 0) { //255 if using 8-bit
			dac_value--;
		} else {
			dac_value = DAC_MAX_VALUE;
		}
		HAL_Delay(2);
	}
}

//void accelerate(int input) {
//	for (int dac_value = 0; dac_value <= input; dac_value++) {
//		//sets DAC_CHANNEL_2 to voltage at dac_value (built-in DAC converts
//		//channel_2 outputting on PA5
//		HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac_value);
//		HAL_Delay(1); //change if its too fast (the LIM blows up)
//	}
//}
//
//void killThrottle() {
//	HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, 0);
//}
