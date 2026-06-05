/*
 * fsm_actions.c
 *
 */

#include "fsm_actions.h"
#include "throttleDriver.h"


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
	WS2812_Init(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
	//WS2812_SetAll(255, 255, 255);
	//WS2812_Start();
	HAL_Delay(1000);
	printf("Finished LED initialization.\r\n");


// INA219
	ina_up_ok = INA219_Init(&ina219_upstream, &hi2c1, INA219_ADDRESS_UPSTREAM);
	if (!ina_up_ok) {
	printf("WARNING: INA219 upstream (0x%02X) init failed\r\n", INA219_ADDRESS_UPSTREAM);
	}

	ina_down_ok = INA219_Init(&ina219_downstream, &hi2c1, INA219_ADDRESS_DOWNSTREAM);
	if (!ina_down_ok) {
	printf("WARNING: INA219 DOWNSTREAM (0x%02X) init failed\r\n", INA219_ADDRESS_DOWNSTREAM);
	}
	printf("Finished INAs initialization.\r\n");

// INA260
	lv_pow = ina260_new(&hi2c1, 0x40);
	if (lv_pow == NULL) {
		// TODO: fail
	}
	if (ina260_wait_until_ready(lv_pow, 1000) != HAL_OK) {
	    // TODO: fail
	}

// CAN -- BMS and VFD
	// can line 1
	HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);


	// CAN1 filter (500)
	FDCAN_FilterTypeDef filter1;
	filter1.IdType = FDCAN_EXTENDED_ID;
	filter1.FilterIndex = 0;
	filter1.FilterType = FDCAN_FILTER_MASK;
	filter1.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
	filter1.FilterID1 = 0;
	filter1.FilterID2 = 0;

	HAL_FDCAN_ConfigFilter(&hfdcan1, &filter1);


	if ((HAL_FDCAN_Start(&hfdcan1) != HAL_OK))
	{
		Error_Handler();
	}

	HAL_FDCAN_ConfigInterruptLines(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, FDCAN_INTERRUPT_LINE0);
	// Activate the notification for new data in FIFO0 for FDCAN1, FIFO1 for FDCAN2
	//this notification triggers the interrupt
	if ((HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK))
	{
		Error_Handler();
	}
	printf("Finished CAN initialization.\r\n");


	printf("===== SENSOR INITIALIZATION COMPLETE =====\r\n");
	printf("  LIDAR: %s | MPU: %s | INA_UP: %s | INA_DOWN: %s\r\n",
	lidar_ok  ? "OK" : "FAIL",
	mpu_ok    ? "OK" : "FAIL",
	ina_up_ok  ? "OK" : "FAIL",
	ina_down_ok ? "OK" : "FAIL");

	osEventFlagsSet(sensorInitFlag, SENSOR_INIT_DONE);
}


bool fault_conditions() {
	/*
	 * Returns true when encountering a fault condition
	 */
	// TODO: FIX THIS ERROR CHECK
	if (sensorData.roll >= 20.0f || sensorData.roll <= -20.0f) {
		printf("FAULT DETECTED! Roll\r\n");
		snprintf(sensorData.message, sizeof(sensorData.message), "FAULT: Roll: %0.2f degrees", sensorData.roll);
		return 1;
	}

	// TODO: FIX THIS ERROR CHECK
	if (sensorData.pitch >= 20.0f || sensorData.pitch <= -20.0f) {
		printf("FAULT DETECTED! Pitch\r\n");
		snprintf(sensorData.message, sizeof(sensorData.message), "FAULT: Pitch: %0.2f degrees", sensorData.pitch);
		return 1;
	}

	if (sensorData.pt_down >= 150) {
		printf("FAULT DETECTED! PNEUMATICS DOWNSTREAM\r\n");
		snprintf(sensorData.message, sizeof(sensorData.message), "FAULT DETECTED! Pneumatics downstream %0.5f", sensorData.pt_down);
		return 1;
	}


	if (sensorData.pt_down <= 0) {
		printf("FAULT DETECTED! PNEUMATICS DOWNSTREAM NOT CONNECTED\r\n");
		snprintf(sensorData.message, sizeof(sensorData.message), "FAULT: Pneumatics downstream %0.5f", sensorData.pt_down);
		return 1;
	}

	if (sensorData.pt_up <= 0) {
		printf("FAULT DETECTED! PNEUMATICS UPSTREAM NOT CONNECTED\r\n");
		snprintf(sensorData.message, sizeof(sensorData.message), "FAULT: Pneumatics upstream %0.5f", sensorData.pt_up);
		return 1;
	}

	for (int i = 0; i < THERMISTOR_COUNT; i++) {
		if (sensorData.thermistors[i] >= 68) {
			printf("FAULT DETECTED! Temperature\r\n");
			snprintf(sensorData.message, sizeof(sensorData.message), "FAULT: Temperature: %0.5f", sensorData.thermistors[i]);
			return 1;
		}
	}

	if (sensorData.lidar_dist < 20) { //TODO: CHANGE THIS
		printf("FAULT DETECTED! LIDAR\r\n");
		snprintf(sensorData.message, sizeof(sensorData.message), "FAULT: LIDAR: %lu", sensorData.lidar_dist);
		return 1;
	}

	// TODO: check battery
//	if (sensorData.pack_volt <= 0.0f) {
//		printf("FAULT DETECTED! BMS not connected\r\n");
//		snprintf(sensorData.message, sizeof(sensorData.message), "FAULT: BMS pack voltage not connected (%.2fV)", sensorData.pack_volt);
//		return 1;
//	}
//	if (sensorData.pack_volt < 60.0f) {
//		printf("FAULT DETECTED! BMS under voltage\r\n");
//		snprintf(sensorData.message, sizeof(sensorData.message), "FAULT: BMS pack under voltage (%.2fV)", sensorData.pack_volt);
//		return 1;
//	}
//	if (sensorData.pack_volt > 100.8f) {
//		printf("FAULT DETECTED! BMS over voltage\r\n");
//		snprintf(sensorData.message, sizeof(sensorData.message), "FAULT: BMS pack over voltage (%.2fV)", sensorData.pack_volt);
//		return 1;
//	}
//	if (sensorData.lowest_cell_volt < 2.8f) {
//		printf("FAULT DETECTED! BMS Cell under voltage\r\n");
//		snprintf(sensorData.message, sizeof(sensorData.message), "FAULT: BMS cell under voltage (%.2fV)", sensorData.lowest_cell_volt);
//		return 1;
//	}
//	if (sensorData.highest_cell_volt > 4.2f) {
//		printf("FAULT DETECTED! BMS cell over voltage\r\n");
//		snprintf(sensorData.message, sizeof(sensorData.message), "FAULT: BMS cell over voltage (%.2fV)", sensorData.highest_cell_volt);
//		return 1;
//	}
//	if (sensorData.pack_soc < 10.0f) {
//		printf("FAULT DETECTED! BMS cell over voltage\r\n");
//		snprintf(sensorData.message, sizeof(sensorData.message), "FAULT: BMS battery low SOC (%.1f%%)", sensorData.pack_soc);
//		return 1;
//	}
//	if (sensorData.highest_temp > 55.0f) {
//		printf("FAULT DETECTED! BMS battery temp too high\r\n");
//		snprintf(sensorData.message, sizeof(sensorData.message), "FAULT: BMS battery temp (%.1fC)", sensorData.highest_temp);
//		return 1;
//	}

	// TODO: check powers
//	if (sensorData.error_code != 0) {
//		snprintf(sensorData.message, sizeof(sensorData.message), "FAULT: VFD error code %lu", sensorData.error_code);
//		return 1;
//	}
//	if (sensorData.motor_temp > 120.0f) {
//		snprintf(sensorData.message, sizeof(sensorData.message), "FAULT: Motor temp (%1.fC)", sensorData.motor_temp);
//		return 1;
//	}
//	if (sensorData.controller_temp > 85.0f) {
//		snprintf(sensorData.message, sizeof(sensorData.message), "FAULT: Controller temp (%1.fC)", sensorData.controller_temp);
//		return 1;
//	}

	return 0;
}

void none_actions() {
	// TODO: Set None color??
	if (HAL_GPIO_ReadPin(GPIOB, Brake_Pin) != GPIO_PIN_SET) {
		HAL_GPIO_WritePin(GPIOB, Brake_Pin, GPIO_PIN_SET); //brakes close
	}
	if (HAL_GPIO_ReadPin(GPIOC, HV_Pin) != GPIO_PIN_RESET) {
		HAL_GPIO_WritePin(GPIOC, HV_Pin, GPIO_PIN_RESET); // HV OFF
	}
}

void init_actions() {
	init_sensors();
	solid_color(PINK);
    HAL_GPIO_WritePin(GPIOB, Brake_Pin, GPIO_PIN_SET); // brakes close
	HAL_GPIO_WritePin(GPIOC, HV_Pin, GPIO_PIN_RESET); // HV OFF
    printf("INIT complete -- waiting for transition\r\n");
}

void load_actions() {
	solid_color(BLUE);
	HAL_GPIO_WritePin(GPIOB, Brake_Pin, GPIO_PIN_RESET); //brakes open
	HAL_GPIO_WritePin(GPIOC, HV_Pin, GPIO_PIN_RESET); // HV OFF
    printf("LOAD complete -- waiting for transition\r\n");
}

int precharge_actions() {
//	solid_color(BLUE);
	if (HAL_GPIO_ReadPin(GPIOB, Brake_Pin) != GPIO_PIN_RESET) {
	    return 0;
	}
	HAL_GPIO_WritePin(GPIOC, HV_Pin, GPIO_PIN_SET); // HV ON
	osDelay(50);

	osMutexAcquire(i2cMutex, osWaitForever);
	DAC_SetValue(25);
	osMutexRelease(i2cMutex);
	osDelay(50);

	printf("PRECHARGE complete -- waiting for transition\r\n");
	return 1;
}

int start_actions() {
	solid_color(GREEN);
	if (HAL_GPIO_ReadPin(GPIOB, Brake_Pin) != GPIO_PIN_RESET) {
	    return 0;
	}
	osMutexAcquire(i2cMutex, osWaitForever);
	DAC_SetValue(410); // throttle 10%
	osMutexRelease(i2cMutex);
	printf("START complete -- waiting for transition\r\n");
	return 1;
}


void stop_actions() {
	solid_color(RED);
	HAL_GPIO_WritePin(GPIOB, Brake_Pin, GPIO_PIN_SET); //brakes close
	DAC_SetValue(25);
	HAL_GPIO_WritePin(GPIOC, HV_Pin, GPIO_PIN_RESET);  // HV OFF
	printf("STOP complete -- waiting for transition\r\n");
}

void fault_actions() {
	solid_color(PURPLE);
	HAL_GPIO_WritePin(GPIOB, Brake_Pin, GPIO_PIN_SET); //brakes close
	printf("FAULT complete -- waiting for transition\r\n");
	DAC_SetValue(25);
	HAL_GPIO_WritePin(GPIOC, HV_Pin, GPIO_PIN_RESET); // HV OFF
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
    s->pod_state = 0;

    s->drive_direction = 0;
    s->encoder_speed = 0;
    s->error_code = 0;
    s->batt_voltage = 0;
    s->motor_curr = 0;
    s->motor_temp = 0;
    s->controller_temp = 0;

	// BMS data
    s->lowest_cell_volt = 0;
    s->highest_cell_volt = 0;
    s->pack_soc = 0;
    s->highest_temp = 0;
    s->pack_volt = 0;
    s->lowest_temp = 0;
    s->current_draw = 0;
    s->relay_status = 0;
    s->bms_test_counter = 0;

    strcpy(s->message, "Sensor Data Cleared");
}
