/*
 * fsm_actions.h
 *
 */

#ifndef INC_FSM_ACTIONS_H_
#define INC_FSM_ACTIONS_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "stm32h7xx_hal.h"
#include "cmsis_os.h"

#include "pod_types.h"
#include "MPU6050.h"
#include "thermistor.h"
#include "ina219.h"
#include "ina260.h"
#include "ws2812b.h"
#include "lidar.h"
#include "time_utils.h"

#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "can.h"
#include "fdcan.h"

#define GUI_CONNECTED (1 << 0)
#define ADC_READY_FLAG (1 << 0)
#define SENSOR_INIT_DONE (1 << 0)
#define THERMISTOR_COUNT 8

extern FSM_t fsm;
extern SensorData sensorData;
extern PreRunStatus preRun;

extern osMutexId_t        sensorMutex;
extern osMutexId_t        i2cMutex;
extern osEventFlagsId_t   sensorInitFlag;
extern osEventFlagsId_t   adcFlag;
extern osEventFlagsId_t   GUIConnectionFlag;

extern INA219_t ina219_upstream;
extern INA219_t ina219_downstream;

extern ina260_t *lv_pow;

extern uint32_t object_distance;

extern uint8_t lidar_ok;
extern uint8_t mpu_ok;
extern uint8_t ina_up_ok;
extern uint8_t ina_down_ok;

extern uint16_t rawValues[THERMISTOR_COUNT];
extern float thermistorValues[THERMISTOR_COUNT];

extern volatile uint8_t convCompleted;
extern volatile uint8_t firstConversionComplete;

/* ── Action function prototypes ─────────────────────────────────────────── */
/**
 * @brief Initialise all sensors
 *        Sets fsm.currentState = FAULT on any failure.
 *        Sets sensorInitFlag SENSOR_INIT_DONE on success.
 */
void init_sensors(void);

/**
 * @brief Check all live fault conditions against current sensor data.
 * @param data  Pointer to the shared SensorData packet.
 * @return true if any fault condition is detected.
 */
bool fault_conditions();

/* State entry actions — called once on entry to each FSM state */
void none_actions();
void init_actions();
void load_actions();
int precharge_actions();
int start_actions();
void stop_actions();
void fault_actions();

/**
 * @brief Zero out a SensorData packet and reset all fields to safe defaults.
 * @param s  Pointer to the SensorData to clear.
 */
void ClearSensorData(SensorData *s);


#endif /* INC_FSM_ACTIONS_H_ */
