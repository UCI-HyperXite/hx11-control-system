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
#include "ws2812b.h"
#include "lidar.h"
#include "led_utils.h"
#include "time_utils.h"

#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

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

extern INA219_t ina219_left;
extern INA219_t ina219_right;
extern uint32_t object_distance;

extern uint8_t lidar_ok;
extern uint8_t mpu_ok;
extern uint8_t ina_left_ok;
extern uint8_t ina_right_ok;

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
 * @brief Run the pre-run checklist and populate the global preRun struct.
 * @param data  Pointer to the shared SensorData packet.
 */
void pre_run_checklist(SensorData *data);

/**
 * @brief Check all live fault conditions against current sensor data.
 * @param data  Pointer to the shared SensorData packet.
 * @return true if any fault condition is detected.
 */
bool fault_conditions(SensorData *data);

/* State entry actions — called once on entry to each FSM state */
void init_actions(SensorData *data);
void load_actions(SensorData *data);
void precharge_actions(SensorData *data);
void start_actions(SensorData *data);
void stop_actions(SensorData *data);
void fault_actions(SensorData *data);
void halt_actions(SensorData *data);

/**
 * @brief Zero out a SensorData packet and reset all fields to safe defaults.
 * @param s  Pointer to the SensorData to clear.
 */
void ClearSensorData(SensorData *s);


#endif /* INC_FSM_ACTIONS_H_ */
