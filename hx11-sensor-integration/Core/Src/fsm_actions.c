/*
 * fsm_actions.c
 *
 */

#include "fsm_actions.h"


void init_sensors(void) {
// Initialize DMA for thermistors
	if (HAL_ADC_GetState(&hadc1) != HAL_ADC_STATE_RESET) {
		HAL_ADC_Stop_DMA(&hadc1);
	}
    memset(rawValues, 0, sizeof(rawValues));
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)rawValues, THERMISTOR_COUNT);

// Find LIDAR
    int okay = 0;
    for (int i = 0; i < 3; i++) {
        osMutexAcquire(i2cMutex, osWaitForever);
        if (HAL_I2C_IsDeviceReady(&hi2c1, 0x62 << 1, 2, 50) == HAL_OK) {
            osMutexRelease(i2cMutex);
            okay = 1;
            break;
        }
        osMutexRelease(i2cMutex);
        osDelay(10);
    }
    if (!okay) {
        fsm.currentState = FAULT;
        fsm.stateEntry = 1;
        printf("LIDAR could not be found.\r\n");
		osMutexAcquire(sensorMutex, osWaitForever);
		snprintf(sensorData.message, sizeof(sensorData.message), "LIDAR could not be found!");
		osMutexRelease(sensorMutex);
        return;
    } else {
    	printf("LIDAR Found\r\n");
    }

//    Initialize LIDAR
    osMutexAcquire(i2cMutex, osWaitForever);
    lidar_init(&hi2c1);
    osMutexRelease(i2cMutex);
    osDelay(25);

    osMutexAcquire(i2cMutex, osWaitForever);
    lidar_config(3);
    osMutexRelease(i2cMutex);

// Initialize MPU
    osMutexAcquire(i2cMutex, osWaitForever);
    mpu_ok = (HAL_I2C_IsDeviceReady(&hi2c1, 0xD0, 2, 50) == HAL_OK) ? 1 : 0;
    if (mpu_ok) {
    	MPU6050_Initialization(&hi2c1);
        printf("Finished MPU initialization.\r\n");
	} else {
		fsm.currentState = FAULT;
		fsm.stateEntry = 1;
		printf("MPU6050 could not be found.\r\n");
		osMutexAcquire(sensorMutex, osWaitForever);
		snprintf(sensorData.message, sizeof(sensorData.message), "MPU6050 could not be found!");
		osMutexRelease(sensorMutex);
		return;
	}
    osMutexRelease(i2cMutex);


//	 THERMISTORS
	printf("THERMS initializing...\r\n");
	firstConversionComplete = 0;

	HAL_StatusTypeDef res = HAL_ADC_Start_DMA(&hadc1, (uint32_t*)rawValues, THERMISTOR_COUNT);
	printf("After ADC start, res=%d\n", res);

	// Wait for first conversion or timeout
	uint32_t timeout = osWaitForever; // or set a reasonable timeout
	osEventFlagsWait(adcFlag, ADC_READY_FLAG, osFlagsNoClear, timeout);
	printf("Finished THERMS initialization.\r\n");


	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)rawValues, THERMISTOR_COUNT);
	if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)rawValues, THERMISTOR_COUNT) != HAL_OK) {
	  printf("❌ ADC DMA failed to start\r\n");
	  // Send errors to POD
	} else {
	  printf("✅ ADC DMA started\r\n");
	  osEventFlagsSet(adcFlag, ADC_READY_FLAG);
	}
	osEventFlagsSet(adcFlag, ADC_READY_FLAG);


//  LED
	printf("LED initializing...\r\n");
	WS2812_Init(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
	//WS2812_SetAll(255, 255, 255);
	WS2812_Start();
	HAL_Delay(1000);
	printf("Finished LED initialization.\r\n");


// INA
	printf("INAs initializing...\r\n");
	ina_up_ok = INA219_Init(&ina219_upstream, &hi2c1, INA219_ADDRESS_40);
	if (!ina_up_ok) {
	printf("WARNING: INA219 upstream (0x%02X) init failed\r\n", INA219_ADDRESS_40);
	}

	ina_down_ok = INA219_Init(&ina219_downstream, &hi2c1, INA219_ADDRESS_41);
	if (!ina_down_ok) {
	printf("WARNING: INA219 DOWNSTREAM (0x%02X) init failed\r\n", INA219_ADDRESS_41);
	}
	printf("Finished INAs initialization.\r\n");


	printf("===== SENSOR INITIALIZATION COMPLETE =====\r\n");
	printf("  LIDAR: %s | MPU: %s | INA_UP: %s | INA_DOWN: %s\r\n",
	lidar_ok  ? "OK" : "FAIL",
	mpu_ok    ? "OK" : "FAIL",
	ina_up_ok  ? "OK" : "FAIL",
	ina_down_ok ? "OK" : "FAIL");

	osEventFlagsSet(sensorInitFlag, SENSOR_INIT_DONE);
}

//void pre_run_checklist(SensorData *data)
//{
//	// testing purposes lolz
////	preRun.sensorsOk = 1;
////	preRun.brakesClosed = 1;
////	preRun.tiltOk = 1;
////	preRun.pneumaticsOk = 1;
////	preRun.batteryOk = 1;
//
//	SensorData localCopy;
//	osMutexAcquire(sensorMutex, osWaitForever);
//	memcpy(&localCopy, data, sizeof(SensorData));
//	osMutexRelease(sensorMutex);
//
//
//    // TODO: GUI comms check
//
//    // sensors initialized -- can be read
//	preRun.sensorsOk = 1;
//
//    // TODO: brakes closed
//
//	// LEDs turned on
//	WS2812_SetAll(128, 0, 64);
//	WS2812_Start();
//	preRun.ledsOk = 1;
//
//    // tilt range check -- roll, pitch
//		// TODO: change bounds vals and error ping
//	if (localCopy.roll >= 23.34 || localCopy.pitch >= 23.34) {
//		preRun.tiltOk = 0;
//		printf("ERROR");
//	} else {
//		preRun.tiltOk = 1;
//	}
//
//    // TODO: pneumatics pressure
//
//    // TODO: battery safe -- within V, I, T range
//
//	preRun.allOk = (
//		preRun.sensorsOk &&
//		preRun.brakesClosed &&
//		preRun.ledsOk &&
//		preRun.tiltOk &&
//		preRun.pneumaticsOk &&
//		preRun.batteryOk
//	);
//}

bool fault_conditions(SensorData *data) {
	/*
	 * Returns true when encountering a fault condition
	 */
	// dynamics
//	if (data->roll >= 23.34 || data->pitch >= 23.34) return 1;

	// TODO: FIX THIS ERROR CHECK
	if (sensorData.roll >= 23.34) {
		printf("FAULT DETECTED! Roll\r\n");
		osMutexAcquire(sensorMutex, osWaitForever);
		snprintf(sensorData.message, sizeof(sensorData.message), "FAULT DETECTED! Roll: %0.5f", sensorData.roll);
		osMutexRelease(sensorMutex);
		return 1;
	}

	// TODO: FIX THIS ERROR CHECK
	if (sensorData.pitch >= 23.34) {
		printf("FAULT DETECTED! Pitch\r\n");
		snprintf(sensorData.message, sizeof(sensorData.message), "FAULT DETECTED! Pitch: %0.5f", sensorData.pitch);
		return 1;
	}

	// TODO: braking (pneumatics)

	// LIM
//	for (int i = 0; i < THERMISTOR_COUNT; i++) {
//		if (data->thermistors[i] >= 80) return 1;
//	}
	for (int i = 0; i < THERMISTOR_COUNT; i++) {
		if (sensorData.thermistors[i] >= 30) {
			printf("FAULT DETECTED! Temperature\r\n");
			snprintf(sensorData.message, sizeof(sensorData.message), "FAULT DETECTED! Temperature: %0.5f", sensorData.thermistors[i]);
			return 1;
		}
	}

	// TODO: battery
	// TODO: powers

	// LiDAR check
//	if (data->lidar_dist > 114) return 1;
	if (sensorData.lidar_dist < 200) {
		printf("FAULT DETECTED! LIDAR\r\n");
		snprintf(sensorData.message, sizeof(sensorData.message), "FAULT DETECTED! LIDAR: %lu", sensorData.lidar_dist);
		return 1;
	}
	return 0;
}

void init_actions(SensorData *data) {
	init_sensors();
	blink_color(255, 20, 147); //pink
    HAL_GPIO_WritePin(GPIOB, Brake_Pin, GPIO_PIN_SET); // brakes close

    // TODO: establish GUI comms again

    // TODO: LV system ON -- calibrate sensors??? what does this mean
//    HAL_GPIO_WritePin(GPIOX, LV_ENABLE_PIN, GPIO_PIN_SET);

    // TODO: send sensor data to GUI
    printf("INIT complete -- waiting for transition\r\n");
}

void load_actions(SensorData *data) {
//    printf("Entering LOAD state\r\n");
	blink_color(0, 0, 255); //blue
	HAL_GPIO_WritePin(GPIOB, Brake_Pin, GPIO_PIN_RESET); //brakes open
    // TODO: LV stays ON
//    HAL_GPIO_WritePin(GPIOX, LV_ENABLE_PIN, GPIO_PIN_SET);

    // TODO: HV OFF
//    HAL_GPIO_WritePin(GPIOX, HV_ENABLE_PIN, GPIO_PIN_RESET);

    // TODO: send sensor data to GUI

    printf("LOAD complete -- waiting for transition\r\n");
}

void precharge_actions(SensorData *data) {
	blink_color(255, 255, 0); //yellow
	// brakes are open

	// TODO: turn on HV sequence

	// TODO: send sensor data to GUI
	printf("PRECHARGE complete -- waiting for transition\r\n");
}

void start_actions(SensorData *data) {
	blink_color(0, 255, 0); //green
	//brakes are open
	//	TODO: StartVFD_LIM();
	//	TODO: SendSensorDataToGUI(data);
	printf("START complete -- waiting for transition\r\n");
}


void stop_actions(SensorData *data) {
	blink_color(255, 0, 0); //red
	HAL_GPIO_WritePin(GPIOB, Brake_Pin, GPIO_PIN_SET); //brakes close
	//	TODO: SetHVPower(OFF);
	//	TODO: SendSensorDataToGUI(data);
	printf("STOP complete -- waiting for transition\r\n");
}

void fault_actions(SensorData *data) {
	blink_color(128, 0, 255); // purple
	HAL_GPIO_WritePin(GPIOB, Brake_Pin, GPIO_PIN_SET); //brakes close
	// TODO: EmergencyRelayCutoff();
	// TODO: StoreFaultInfo(data);

	printf("FAULT complete -- waiting for transition\r\n");
}

void ClearSensorData(SensorData *s)
{
	// Reset all the numbers
    memset(s, 0, sizeof(SensorData));
    s->start_marker = 0xAA;
    s->lidar_dist   = 0;
    s->roll         = 0.0f;
    s->pitch        = 0.0f;

    for (int i = 0; i < 8; i++)
        s->thermistors[i] = 0.0f;

    s->pt_up = 0;
    s->pt_down = 0;
    s->lv_batt = 0;
    s->hv_batt = 0;
    s->hv_batt_temp = 0;
    s->batt_soc = 0;
    s->lim_volt = 0;
    s->lim_curr = 0;
    s->imd = 0;
    s->pod_state = 0;

    strcpy(s->message, "Sensor Data Cleared");
}
