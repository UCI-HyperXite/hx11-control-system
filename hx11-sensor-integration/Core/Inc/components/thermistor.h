///*
// * thermistor.h
// *
// *  Created on: Oct 23, 2025
// *      Author: leslie
// */
#ifndef INC_THERMISTOR_H_
#define INC_THERMISTOR_H_
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Declare a C-callable function
float readThermistor(void);
uint32_t read_ADC_value(void);
float ntc_convertToC(uint32_t adcValue);
#ifdef __cplusplus
}
#endif
#endif /* INC_THERMISTOR_H_ */
