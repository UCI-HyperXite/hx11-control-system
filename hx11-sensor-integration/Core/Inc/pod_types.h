/*
 * pod_types.h
 *
 */

#ifndef INC_POD_TYPES_H_
#define INC_POD_TYPES_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct __attribute__((packed)) {
	uint8_t start_marker;
	uint32_t lidar_dist;
	float roll, pitch;            //MPU
	float thermistors[8];
	float pt_up, pt_down;         //ina219, left and right brake
	float lv_batt;                //ina260

	// VFD
	uint8_t drive_direction,
	error_code;

	double encoder_speed,
	batt_voltage,
	motor_curr,
	motor_temp,
	controller_temp;

	// BMS
	uint8_t relay_status, bms_test_counter;
	double lowest_cell_volt,
	highest_cell_volt,
	pack_soc,
	highest_temp,
	pack_volt,
	lowest_temp;

	uint8_t dis_en_status;

	uint8_t pod_state;
	char message[100];
} SensorData;

typedef enum {
	NONE,
	GUI_OK,
	INIT,
	LOAD,
	PRECHARGE,
	START,
	STOP,
	FAULT,
} pod_state_t;

typedef enum {
    CMD_NONE,
	CMD_OK,
	CMD_INIT,
    CMD_LOAD,
	CMD_PRECHARGE,
    CMD_STOP,
	CMD_FAULT
} GUICommand;

typedef struct {
	pod_state_t currentState;
	pod_state_t previousState;
    uint8_t stateEntry; // 1 when entering new state
} FSM_t;

typedef struct {
    uint8_t guiCommsOk;
    uint8_t sensorsOk;
    uint8_t brakesClosed;
    uint8_t ledsOk;
    uint8_t tiltOk; // roll, pitch
    uint8_t pneumaticsOk;
    uint8_t batteryOk;
    uint8_t allOk;
} PreRunStatus;

#endif /* INC_POD_TYPES_H_ */
