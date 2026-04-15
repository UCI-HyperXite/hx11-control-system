#include "thermistor.h"
#include "main.h"
#include <math.h>
#define _NTC_R_SERIES         10000.0f
#define _NTC_R_NOMINAL        10000.0f
#define _NTC_TEMP_NOMINAL     25.0f
#define _NTC_ADC_MAX          65535 //  16bit
#define _NTC_BETA             3950
extern ADC_HandleTypeDef hadc1;
// readThermistor calls the other two functions in this file
// gets the ADC_value and converts it to temp in Celsius, returns temp
float readThermistor(void){
	uint32_t adcValue = read_ADC_value();
	float tempC = ntc_convertToC(adcValue);
	return tempC;
}
// ----------------------------------------------------
// Reads ADC value from configured channel
// ----------------------------------------------------
uint32_t read_ADC_value(void){
	uint32_t adcValue = 0;
	HAL_ADC_Start(&hadc1); 									// Start ADC conversion
	if(HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
	  {
		  adcValue = HAL_ADC_GetValue(&hadc1);
	  }
	HAL_ADC_Stop(&hadc1);
	return adcValue;
}
// ----------------------------------------------------
// Converts ADC value → Celsius
// ----------------------------------------------------
float ntc_convertToC(uint32_t adcValue)
{
	if (adcValue == 0) adcValue = 1;
	if (adcValue >= _NTC_ADC_MAX) adcValue = _NTC_ADC_MAX - 1;
	float rntc = (float)_NTC_R_SERIES / (((float)_NTC_ADC_MAX / (float)adcValue ) - 1.0f);
	float temp;
	temp = rntc / (float)_NTC_R_NOMINAL;
	temp = logf(temp);
	temp /= (float)_NTC_BETA;
	temp += 1.0f / ((float)_NTC_TEMP_NOMINAL + 273.15f);
	temp = 1.0f / temp;
	temp -= 273.15f;
	return temp;
}
