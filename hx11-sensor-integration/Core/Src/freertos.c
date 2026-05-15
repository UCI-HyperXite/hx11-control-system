/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "fsm_actions.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
FSM_t fsm = {
	.currentState = NONE,
	.previousState = NONE,
	.stateEntry = 1
};

SensorData sensorData;
PreRunStatus preRun;

uint16_t rawValues[THERMISTOR_COUNT];
float thermistorValues[THERMISTOR_COUNT];

osMutexId_t sensorMutex;
osMutexId_t i2cMutex;
osEventFlagsId_t GUIConnectionFlag;
osEventFlagsId_t sensorInitFlag;
osEventFlagsId_t adcFlag;

volatile uint8_t guiCommand = 0;

INA219_t ina219_upstream;
INA219_t ina219_downstream;
uint32_t object_distance;

uint8_t lidar_ok = 0;
uint8_t mpu_ok = 0;
uint8_t ina_up_ok = 0;
uint8_t ina_down_ok = 0;

volatile uint8_t convCompleted = 0;
volatile uint8_t firstConversionComplete = 0;

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	if (hadc->Instance == ADC1) {
		convCompleted = 1;
		if (!firstConversionComplete) {
			firstConversionComplete = 1;
			osEventFlagsSet(adcFlag, ADC_READY_FLAG);
		}
	}
}

uint8_t rxBuffer[1];
uint32_t lastESPHeartbeat = 0;
volatile uint8_t currentCmd = 0;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == UART7) {
        uint8_t cmd = rxBuffer[0];
        HAL_UART_Receive_IT(&huart7, rxBuffer, 1);
		lastESPHeartbeat = HAL_GetTick();
		currentCmd = cmd;
    }
}

/* USER CODE END Variables */
/* Definitions for lidarTask */
osThreadId_t lidarTaskHandle;
const osThreadAttr_t lidarTask_attributes = {
  .name = "lidarTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for mpuTask */
osThreadId_t mpuTaskHandle;
const osThreadAttr_t mpuTask_attributes = {
  .name = "mpuTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for thermistorsTask */
osThreadId_t thermistorsTaskHandle;
const osThreadAttr_t thermistorsTask_attributes = {
  .name = "thermistorsTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
};
/* Definitions for INATask */
osThreadId_t INATaskHandle;
const osThreadAttr_t INATask_attributes = {
  .name = "INATask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal3,
};
/* Definitions for telemetryTask */
osThreadId_t telemetryTaskHandle;
const osThreadAttr_t telemetryTask_attributes = {
  .name = "telemetryTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for commandTask */
osThreadId_t commandTaskHandle;
const osThreadAttr_t commandTask_attributes = {
  .name = "commandTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal3,
};
/* Definitions for fsmTask */
osThreadId_t fsmTaskHandle;
const osThreadAttr_t fsmTask_attributes = {
  .name = "fsmTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartLidarTask(void *argument);
void StartMPUTask(void *argument);
void StartThermistorsTask(void *argument);
void StartINATask(void *argument);
void StartTelemetryTask(void *argument);
void StartCommandTask(void *argument);
void StartFSMTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
	sensorMutex      = osMutexNew(NULL);
	i2cMutex         = osMutexNew(NULL);
	GUIConnectionFlag = osEventFlagsNew(NULL);
	sensorInitFlag   = osEventFlagsNew(NULL);
	adcFlag          = osEventFlagsNew(NULL);
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of lidarTask */
  lidarTaskHandle = osThreadNew(StartLidarTask, NULL, &lidarTask_attributes);

  /* creation of mpuTask */
  mpuTaskHandle = osThreadNew(StartMPUTask, NULL, &mpuTask_attributes);

  /* creation of thermistorsTask */
  thermistorsTaskHandle = osThreadNew(StartThermistorsTask, NULL, &thermistorsTask_attributes);

  /* creation of INATask */
  INATaskHandle = osThreadNew(StartINATask, NULL, &INATask_attributes);

  /* creation of telemetryTask */
  telemetryTaskHandle = osThreadNew(StartTelemetryTask, NULL, &telemetryTask_attributes);

  /* creation of commandTask */
  commandTaskHandle = osThreadNew(StartCommandTask, NULL, &commandTask_attributes);

  /* creation of fsmTask */
  fsmTaskHandle = osThreadNew(StartFSMTask, NULL, &fsmTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartLidarTask */
/**
  * @brief  Function implementing the lidarTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartLidarTask */
void StartLidarTask(void *argument)
{
  /* USER CODE BEGIN StartLidarTask */
	printf("LIDAR Waiting\r\n");
	osEventFlagsWait(GUIConnectionFlag, GUI_CONNECTED, osFlagsWaitAll | osFlagsNoClear, osWaitForever);
	osEventFlagsWait(sensorInitFlag, SENSOR_INIT_DONE, osFlagsNoClear, osWaitForever);

	SensorData *data = (SensorData *)argument;

	for(;;)
	{
		osEventFlagsWait(GUIConnectionFlag, GUI_CONNECTED, osFlagsWaitAll | osFlagsNoClear, osWaitForever);
		osEventFlagsWait(sensorInitFlag, SENSOR_INIT_DONE, osFlagsNoClear, osWaitForever);

		osMutexAcquire(i2cMutex, osWaitForever);
		object_distance = retrieve_lidar_distance();
		osMutexRelease(i2cMutex);

		// TODO: REMOVE THIS LATER WHEN LIDAR WORKS
//		if (object_distance < 0 || object_distance > 4000)
//		{
//		    printf("LIDAR ERROR: %d\r\n", object_distance);
////		    object_distance = 0;
//		}

		osMutexAcquire(sensorMutex, osWaitForever);
		sensorData.lidar_dist = object_distance;
		data->lidar_dist = object_distance;
		osMutexRelease(sensorMutex);

		osDelay(500);
	}
  /* USER CODE END StartLidarTask */
}

/* USER CODE BEGIN Header_StartMPUTask */
/**
* @brief Function implementing the mpuTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartMPUTask */
void StartMPUTask(void *argument)
{
  /* USER CODE BEGIN StartMPUTask */
	//	printf("MPU Waiting\r\n");
	osEventFlagsWait(GUIConnectionFlag, GUI_CONNECTED, osFlagsWaitAll | osFlagsNoClear, osWaitForever);
	osEventFlagsWait(sensorInitFlag, SENSOR_INIT_DONE, osFlagsNoClear, osWaitForever);

	float gyro_x_bias = 0;
	float gyro_y_bias = 0;
	int32_t bias_x_acc = 0, bias_y_acc = 0;
	for (int i = 0; i < 200; i++)
	{
	osMutexAcquire(i2cMutex, osWaitForever);

	if (MPU6050_DataReady())
	{
		MPU6050_ProcessData(&MPU6050);

		bias_x_acc += MPU6050.gyro_x;
		bias_y_acc += MPU6050.gyro_y;
	}

	osMutexRelease(i2cMutex);

	osDelay(5);
	}
	gyro_x_bias = bias_x_acc / 200.0f;
	gyro_y_bias = bias_y_acc / 200.0f;

  /* Infinite loop */
	for(;;)
	{
		osEventFlagsWait(GUIConnectionFlag, GUI_CONNECTED, osFlagsWaitAll | osFlagsNoClear, osWaitForever);
		osEventFlagsWait(sensorInitFlag, SENSOR_INIT_DONE, osFlagsNoClear, osWaitForever);

		osMutexAcquire(i2cMutex, osWaitForever);
		int ready = MPU6050_DataReady();
		if (ready) {
		  MPU6050_ProcessData(&MPU6050);
		}
		osMutexRelease(i2cMutex);

		if (ready) {
		  float gx = MPU6050.gyro_x - gyro_x_bias;
		  float gy = MPU6050.gyro_y - gyro_y_bias;

		  osMutexAcquire(sensorMutex, osWaitForever);
		  sensorData.roll = gx;
		  sensorData.pitch = gy;
		//		  data->roll = gx;
		//		  data->pitch = gy;
		  osMutexRelease(sensorMutex);
		}
		osDelay(20);
	}
  /* USER CODE END StartMPUTask */
}

/* USER CODE BEGIN Header_StartThermistorsTask */
/**
* @brief Function implementing the thermistorsTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartThermistorsTask */
void StartThermistorsTask(void *argument)
{
  /* USER CODE BEGIN StartThermistorsTask */
	printf("Thermistor Waiting\r\n");
	osEventFlagsWait(GUIConnectionFlag, GUI_CONNECTED, osFlagsWaitAll | osFlagsNoClear, osWaitForever);
	osEventFlagsWait(adcFlag, ADC_READY_FLAG, osFlagsNoClear, osWaitForever);
	osEventFlagsWait(sensorInitFlag, SENSOR_INIT_DONE, osFlagsNoClear, osWaitForever);

	uint32_t startTime = HAL_GetTick();
	uint32_t hours = 0;
	uint32_t minutes = 0;
	uint32_t seconds = 0;
	uint32_t time_elapsed = 0;

//	SensorData *data = (SensorData *)argument;
  /* Infinite loop */
	for(;;)
	{
		osEventFlagsWait(GUIConnectionFlag, GUI_CONNECTED, osFlagsWaitAll | osFlagsNoClear, osWaitForever);
		osEventFlagsWait(sensorInitFlag, SENSOR_INIT_DONE, osFlagsNoClear, osWaitForever);
//		printf("Thermistors Running\r\n");
		if (convCompleted) {
//			printf("Thermistors Written\r\n");
			convCompleted = 0;
			time_elapsed = HAL_GetTick() - startTime;
			convert_millis_to_hms(time_elapsed, &hours, &minutes, &seconds);
			for (int i=0; i<THERMISTOR_COUNT; i++) {
				thermistorValues[i] = ntc_convertToC(rawValues[i]);;
			}

			osMutexAcquire(sensorMutex, osWaitForever);
			memcpy(sensorData.thermistors, thermistorValues, sizeof(sensorData.thermistors));
			//memcpy(data->thermistors, thermistorValues, sizeof(data->thermistors));
			osMutexRelease(sensorMutex);
		}
		osDelay(100);
	  }
  /* USER CODE END StartThermistorsTask */
}

/* USER CODE BEGIN Header_StartINATask */
/**
* @brief Function implementing the INATask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartINATask */
void StartINATask(void *argument)
{
  /* USER CODE BEGIN StartINATask */
	  /* USER CODE BEGIN StartINATask */
	printf("INA Waiting\r\n");
	osEventFlagsWait(GUIConnectionFlag, GUI_CONNECTED, osFlagsWaitAll | osFlagsNoClear, osWaitForever);
	osEventFlagsWait(sensorInitFlag, SENSOR_INIT_DONE, osFlagsNoClear, osWaitForever);
	SensorData *data = (SensorData *)argument;
	uint16_t current_up, current_down;

	  /* Infinite loop */
	  for(;;)
	  {
		  osEventFlagsWait(GUIConnectionFlag, GUI_CONNECTED, osFlagsWaitAll | osFlagsNoClear, osWaitForever);
		  osEventFlagsWait(sensorInitFlag, SENSOR_INIT_DONE, osFlagsNoClear, osWaitForever);

//		  printf("INA Running\r\n");
		  osMutexAcquire(i2cMutex, osWaitForever);
		  current_up = 1;
		  current_up = INA219_ReadCurrent(&ina219_upstream);  //TODO: UPDATE CURRENT_UP
		  osMutexRelease(i2cMutex);

		  osMutexAcquire(i2cMutex, osWaitForever);
		  current_down = 2;
		  current_down = INA219_ReadCurrent(&ina219_downstream);  //TODO: UPDATE CURRENT_DOWN
		  osMutexRelease(i2cMutex);
		  /* END */

		  osMutexAcquire(sensorMutex, osWaitForever);
		  sensorData.pt_up = current_up;
		  sensorData.pt_down = current_down;
		  data->pt_up = current_up;
		  data->pt_down = current_down;
		  osMutexRelease(sensorMutex);

		  osDelay(200);
	  }
  /* USER CODE END StartINATask */
}

/* USER CODE BEGIN Header_StartTelemetryTask */
/**
* @brief Function implementing the telemetryTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTelemetryTask */
void StartTelemetryTask(void *argument)
{
  /* USER CODE BEGIN StartTelemetryTask */
	/* Infinite loop */
	//	char uart_tx_buff[100];
	//	printf("Telemetry Waiting\r\n");
		osEventFlagsWait(GUIConnectionFlag, GUI_CONNECTED, osFlagsWaitAll | osFlagsNoClear, osWaitForever);

		for(;;)
		{
			osEventFlagsWait(GUIConnectionFlag, GUI_CONNECTED, osFlagsWaitAll | osFlagsNoClear, osWaitForever);
	//		sprintf(uart_tx_buff, "Telemetry Sending NOW\r\n");
	//		HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)uart_tx_buff, strlen(uart_tx_buff), 100);

			osMutexAcquire(sensorMutex, osWaitForever);
			sensorData.start_marker = 0xAA;
			HAL_UART_Transmit(&huart7, (uint8_t*)&sensorData, sizeof(SensorData), 100);
			osMutexRelease(sensorMutex);

	//		if (HAL_UART_GetState(&huart7) == HAL_UART_STATE_READY)
	//		{
			// TODO: CAN'T DO NON BLOCKING TRANSMIT UNTIL SET UP UART7 TX DMA IN IOC
	//		  HAL_UART_Transmit_DMA(&huart7, (uint8_t*)&sensorData, sizeof(SensorData));
	//		  sprintf(uart_tx_buff, "UART7 Sent %d bytes successfully\r\n", sizeof(SensorData));
	//		  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)uart_tx_buff, strlen(uart_tx_buff), 100);
	//		}
		  osDelay(100);
		}
  /* USER CODE END StartTelemetryTask */
}

/* USER CODE BEGIN Header_StartCommandTask */
/**
* @brief Function implementing the commandTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCommandTask */
void StartCommandTask(void *argument)
{
  /* USER CODE BEGIN StartCommandTask */
  /* Infinite loop */
	printf("Start Receiving Commands!!\r\n");
	HAL_UART_Receive_IT(&huart7, rxBuffer, 1);
	char msg[50];
	uint8_t prev_cmd = 0;
	const char *cmdNames[] = {
		"NONE", "GUI_OK", "INIT", "LOAD", "START", "STOP", "FAULT"
	};

	for(;;)
	{
		if (HAL_GetTick() - lastESPHeartbeat > 2000)
		{
		  osEventFlagsClear(GUIConnectionFlag, GUI_CONNECTED);
		  currentCmd = 0;
		}

		uint8_t cmd = currentCmd;

		if (cmd != prev_cmd) {
			prev_cmd = cmd;
			fsm.previousState = fsm.currentState;
			fsm.stateEntry = 1;

			switch (cmd) {
			case 0:
				osEventFlagsClear(GUIConnectionFlag, GUI_CONNECTED);
				osEventFlagsClear(sensorInitFlag, SENSOR_INIT_DONE);
				fsm.currentState = NONE;
				break;
			case 1:
				osEventFlagsSet(GUIConnectionFlag, GUI_CONNECTED);
				osEventFlagsClear(sensorInitFlag, SENSOR_INIT_DONE);
				fsm.currentState = GUI_OK;
				break;
			case 2:
				osEventFlagsSet(GUIConnectionFlag, GUI_CONNECTED);
				fsm.currentState = INIT;
				osEventFlagsClear(sensorInitFlag, SENSOR_INIT_DONE);
				break;
			case 3:
				osEventFlagsSet(GUIConnectionFlag, GUI_CONNECTED);
				fsm.currentState = LOAD;
				break;
			case 4:
				osEventFlagsSet(GUIConnectionFlag, GUI_CONNECTED);
				fsm.currentState = START;
				// TODO: START, WAIT FOR STOP
				break;
			case 5:
				osEventFlagsSet(GUIConnectionFlag, GUI_CONNECTED);
				fsm.currentState = STOP;
				// TODO: STOP THE POD!! WAIT FOR LOAD CMD
				break;
			case 6:
				osEventFlagsSet(GUIConnectionFlag, GUI_CONNECTED);
				fsm.currentState = FAULT;
				// TODO: FAULT ACK, WAIT FOR INIT OR LOAD (depends on error)
				break;
			default:
				osEventFlagsClear(GUIConnectionFlag, GUI_CONNECTED);
				osEventFlagsClear(sensorInitFlag, SENSOR_INIT_DONE);
				fsm.currentState = NONE;
				// TODO: STOP POD
				break;
			}

			sprintf(msg, "NEW CMD: %d\r\n", cmd);
			HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)msg, strlen(msg), 100);

			const char *cmdName = (cmd < sizeof(cmdNames) / sizeof(cmdNames[0])) ? cmdNames[cmd] : "UNKNOWN";

			osMutexAcquire(sensorMutex, osWaitForever);
			snprintf(sensorData.message, sizeof(sensorData.message), "Received %s command", cmdName);
			osMutexRelease(sensorMutex);
		}

	  osDelay(10);
  }
  /* USER CODE END StartCommandTask */
}

/* USER CODE BEGIN Header_StartFSMTask */
/**
* @brief Function implementing the fsmTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartFSMTask */
void StartFSMTask(void *argument)
{
  /* USER CODE BEGIN StartFSMTask */
	// Wait until the first GUI connection is alive
	printf("In FSM task, waiting for connect");
	osEventFlagsWait(GUIConnectionFlag, GUI_CONNECTED, osFlagsWaitAll | osFlagsNoClear, osWaitForever);

  /* Infinite loop */
	for(;;)
	{
		// Check if GUI is alive
		uint32_t flags = osEventFlagsWait(GUIConnectionFlag, GUI_CONNECTED, osFlagsWaitAll | osFlagsNoClear, 100);
		if (!flags & !GUI_CONNECTED) {
			/* If the GUI is not connected, ensure the pod is stopped. */
			// TODO: If sensors are not initialized
			// TODO: Brakes/relay
			// TODO: HV system

			fsm.currentState = NONE;
			printf("GUI Not Connected");
			continue;
		}

		// GUI IS ALIVE

		// CHECK FOR FAULT CONDITIONS
		flags = osEventFlagsGet(sensorInitFlag);
		if (flags & SENSOR_INIT_DONE) {
			// If sensors are initialized, check fault conditions
			if (fsm.currentState != FAULT) {
				bool fault_found = fault_conditions(&sensorData);
				if (fault_found) {
					fsm.currentState = FAULT;
					fsm.stateEntry = 1;
				}
			}
		}

		// CHANGE STATES ON ENTRY
		if (fsm.stateEntry == 1) {
			fsm.stateEntry = 0;
			switch (fsm.currentState) {
			case NONE:
				// TODO: STOP POD GO WAIT FOR GUI CONNECTION
				// TODO: Ensure pod is not moving
				printf(">NONE\r\n");
				break;
			case GUI_OK:
				// TODO: GUI OK WAIT FOR INIT
				// TODO: make sure pod not running
				osMutexAcquire(sensorMutex, osWaitForever);
				ClearSensorData(&sensorData);
				osMutexRelease(sensorMutex);
				printf(">GUI_OK\r\n");
				break;
			case INIT:
				// TODO: INIT, THEN WAIT FOR LOAD
				init_actions(&sensorData);
				printf(">INIT\r\n");
				break;
			case LOAD:
				// TODO: LOAD -> PRECHARGE
				// TODO: Brakes
				load_actions(&sensorData);

				fsm.previousState = fsm.currentState;
				fsm.currentState = PRECHARGE;
				fsm.stateEntry = 1;
				printf(">LOAD\r\n");
				break;
			case PRECHARGE:
				// TODO: PRECHARGE, THEN WAIT FOR START

				// TODO: Turn on HV
				// TODO: Wait for V/I to stabilize
				// TODO: Set Flag
				precharge_actions(&sensorData);
				printf(">PRECHARGE\r\n");
				break;
			case START:
				// TODO: Start VFD + LIM
				start_actions(&sensorData);
				printf(">START\r\n");
				break;
			case STOP:
				// TODO: Brakes
				stop_actions(&sensorData);
				// TODO: Turn off HV
				printf(">STOP\r\n");
				break;
			case FAULT:
				// TODO: Brakes
				// TODO: Turn off HV

				fault_actions(&sensorData);
				printf(">FAULT\r\n");

				break;
			}

			// Record change of pod state
			osMutexAcquire(sensorMutex, osWaitForever);
			sensorData.pod_state = (uint8_t)fsm.currentState;
			osMutexRelease(sensorMutex);
		}

	    //run_pre_run_checklist(&sensorData);

//		SensorData localCopy;

//		osMutexAcquire(sensorMutex, osWaitForever);
//		memcpy(&localCopy, &sensorData, sizeof(SensorData));
//		osMutexRelease(sensorMutex);

//		bool auto_trigger = fault_conditions(&localCopy);

//		uint8_t pendingCmd;
//		bool hasCommand = false; // checks to see if queue is empty



//		switch(fsm.currentState) {
//		case GUI_OK:
//			if (fsm.stateEntry) {
//				fsm.stateEntry = 0;
//			}
//			// TODO: ENSURE EVERYTHING IS OFF
//
//		case INIT:
//			if (fsm.stateEntry) {
//				fsm.stateEntry = 0;
//				init_actions(&sensorData);
//			}

			// manual GUI trigger
//			if(preRun.allOk && hasCommand && pendingCmd == CMD_LOAD) {
//				//guiCommand = CMD_NONE; // freertos message queue FIFO buffer -- race condition
//				fsm.previousState = fsm.currentState;
//				fsm.currentState = LOAD;
//				fsm.stateEntry = 1;
//			}
//			break;
//		case LOAD:
//			if (fsm.stateEntry) {
//				fsm.stateEntry = 0;
//				load_actions(&sensorData);
//			}

			// automatic trigger
//			fsm.previousState = fsm.currentState;
//			if (auto_trigger) { // this state passes through
//				fsm.currentState = FAULT;
//				fsm.stateEntry = 1;
//			} else if(preRun.allOk && hasCommand && pendingCmd == CMD_PRECHARGE) { // manual GUI trigger
//				fsm.currentState = PRECHARGE;
//				fsm.stateEntry = 1;
//			} else if(preRun.allOk && hasCommand && pendingCmd == CMD_STOP) { // manual GUI trigger
//				fsm.currentState = STOP;
//				fsm.stateEntry = 1;
//			} else {
//				fsm.currentState = LOAD;
//				fsm.stateEntry = 1;
//			}
//			break;
//		case PRECHARGE:
//			if (fsm.stateEntry) {
//				precharge_actions(&sensorData);
//				fsm.stateEntry = 0;
//			}
//
//			fsm.previousState = fsm.currentState;
//			if (auto_trigger) {
//				fsm.currentState = FAULT;
//				fsm.stateEntry = 1;
//			}
			// TODO: automatic trigger -- if voltage/current stabilize
  //			else if (voltage_current) {
  //				fsm.currentState = START;
  //		        fsm.stateEntry = 1;
  //			}
//			else if(hasCommand && pendingCmd == CMD_STOP) { // manual GUI trigger
//				fsm.currentState = STOP;
//				fsm.stateEntry = 1;
//			}
//			break;
//		case START:
//			if (fsm.stateEntry) {
//				start_actions(&sensorData);
//				fsm.stateEntry = 0;
//			}
//
//			fsm.previousState = fsm.currentState;
//			if (auto_trigger) {
//				fsm.currentState = FAULT;
//				fsm.stateEntry = 1;
//			} else if(hasCommand && pendingCmd == CMD_STOP) { // manual GUI trigger
//				fsm.currentState = STOP;
//				fsm.stateEntry = 1;
//			}
//			break;
//		case FAULT:
//			if (fsm.stateEntry) {
//				fault_actions(&sensorData);
//				fsm.stateEntry = 0;
//			}
//
//			fsm.previousState = fsm.currentState;
//			fsm.currentState = FAULT;  // TODO: I CHANGED THIS
//			fsm.stateEntry = 1;
//			break;
//		case HALT:
//			if (fsm.stateEntry) {
//				halt_actions(&sensorData);
//				fsm.stateEntry = 0;
//			}
//
//			fsm.previousState = fsm.currentState;
//			if (hasCommand && pendingCmd == CMD_INIT) {
//				fsm.currentState = INIT;
//				fsm.stateEntry = 1;
//			}
//			break;
//		case STOP:
//			if (fsm.stateEntry) {
//				stop_actions(&sensorData);
//				fsm.stateEntry = 0;
//			}
//
//			fsm.previousState = fsm.currentState;
//			if (auto_trigger) {
//				fsm.currentState = FAULT;
//				fsm.stateEntry = 1;
//			}
//			else if (hasCommand && pendingCmd == CMD_INIT) {
//				fsm.currentState = INIT;
//				fsm.stateEntry = 1;
//			} else if (hasCommand && pendingCmd == CMD_LOAD) {
//				fsm.currentState = LOAD;
//				fsm.stateEntry = 1;
//			}
//			break;
//		}
		osDelay(50);
	}
  /* USER CODE END StartFSMTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/* USER CODE END Application */

