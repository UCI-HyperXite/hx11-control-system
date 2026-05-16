#ifndef CAN
#define CAN

#include <stdio.h>
#include <stdint.h>

#include "stm32h7xx_hal.h"

#define CAN_QUEUE_SIZE 32
#define UNKNOWN_LOG_SIZE 32

#define IDEAL_LOW_CELL_VOLTAGE 3 //needs to be configured in bms to send a fault at this value
#define IDEAL_HIGH_CELL_VOLTAGE 4.5 //a cell voltage shouldn't increase above this value
#define IDEAL_HIGH_TEMP 67 //obtain value on saturday
#define IDEAL_LOWEST_TEMP 67 //obtain value on saturday


typedef struct {
	uint32_t id;
	uint8_t data[8];
} CAN_Message;


typedef struct{
	// direction
	uint8_t drivingDirection;
	// in m/s
	double encoderSpeed;
	// error code (units/decoding not done)
	uint8_t errorCode;
	// in V
	double batteryVoltage;
	// in A
	double motorCurrent;
	// in C (to be converted to F)
	double motorTemp;
	// in C (to be converted to F)
	double controllerTemp;
} VFD_CAN_Data;

extern VFD_CAN_Data vfdData;


typedef struct{
	// in V
	double lowestCellVoltage;
	// in V
	double highestCellVoltage;
	double packSOC;
	double highestTemp; //signed, [1C]
	uint8_t bmsTestCounter;

	//message id 2
	uint8_t relayStatus; //each bit interpreted, 1 == high, 0 == low
	/*
	Bit #1 (0x01): Discharge relay enabled
	Bit #2 (0x02): Charge relay enabled
	Bit #3 (0x04): Charger safety enabled
	Bit #4 (0x08): Malfunction indicator active (DTC status)
	Bit #5 (0x10): Multi-Purpose Input signal status
	Bit #6 (0x20): Always-on signal status
	Bit #7 (0x40): Is-Ready signal status
	Bit #8 (0x80): Is-Charging signal status
	*/
	double packVoltage; //2 bytes, [0.1V]
	double lowestTemp; //signed [1C]
}BMS_CAN_Data;

extern BMS_CAN_Data bmsData;

typedef struct{
	double insulationResistance;
	double iso_status;
	double imd_counter;
	double imd_warnings;
	/* 1 == high/true, 0 == low/false
	Order: 5 4 3 2 1 0
	Bit 0: true = Device error active
	Bit 1: true = HV_pos connection failure
	Bit 2: true = HV_neg connection failure
	Bit 3: true = Earth connection failure
	Bit 4: true = Iso alarm (resistance value below threshold error)
	Bit 5: true = Iso warning (resistance value below threshold warning)
	 */
	double deviceActivity;
}IMD_CAN_Data;

extern IMD_CAN_Data imdData;

extern CAN_Message canQ250[CAN_QUEUE_SIZE]; //declaring array of can messages
extern volatile int head250;
extern volatile int tail250;

extern CAN_Message canQ500[CAN_QUEUE_SIZE]; //declaring array of can messages
extern volatile int head500;
extern volatile int tail500;


double concatenate(uint8_t x, uint8_t y);

double rpm_to_ms(double rpm);

void process_CAN250_msgs(VFD_CAN_Data *vfdData);

void process_CAN500_msgs(BMS_CAN_Data *bmsData, IMD_CAN_Data *imdData);

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan1, uint32_t RxFifo0ITs);// RxFifo0ITs using interrupt 0

void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan2, uint32_t RxFifo1ITs);

#endif
