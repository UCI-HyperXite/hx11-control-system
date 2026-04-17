/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "MPU6050.h"
#include "thermistor.h"
#include "ina219.h"
#include "ws2812b.h"
#include "lidar.h"
#include "peripherals.h"
#include "time_utils.h"

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
  .stack_size = 18 * 4,
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
  .priority = (osPriority_t) osPriorityLow,
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void StartLidarTask(void *argument);
void StartMPUTask(void *argument);
void StartThermistorsTask(void *argument);
void StartINATask(void *argument);
void StartTelemetryTask(void *argument);
void StartCommandTask(void *argument);
void StartFSMTask(void *argument);

/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
volatile uint8_t convCompleted = 0;
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	if (hadc->Instance == ADC1) {
		convCompleted = 1;
	}
}

uint8_t rxBuffer[1];
volatile uint8_t newCommandFlag = 0;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == UART7) {
        newCommandFlag = 1;
        HAL_UART_Receive_IT(&huart7, rxBuffer, 1);
    }
}

//#define CMD_NONE 0
//#define CMD_OK 1
//#define CMD_LOAD 2
//#define CMD_PRECHARGE 3
//#define CMD_STOP 4

typedef struct __attribute__((packed)) {
	uint8_t start_marker;
	uint32_t lidar_dist;
	float roll, pitch;            //MPU
	float thermistors[8];
	float pt_up, pt_down;         //ina219, left and right brake
	float lv_batt;                //ina260
	float hv_batt_temp, hv_batt;  //BMS
	float batt_soc;               //Battery state of charge
	float lim_volt, lim_curr;
	float imd;  				  // IMD status
	uint8_t pod_state;
	char message[100];
} SensorData;


typedef enum {
	INIT,
	LOAD,
	PRECHARGE,
	START,
	STOP,
	FAULT,
	HALT,
} pod_status;

typedef enum {
    CMD_NONE,
	CMD_INIT,
    CMD_LOAD,
    CMD_PRECHARGE,
    CMD_STOP
} GUICommand;

typedef struct {
    pod_status currentState;
    pod_status previousState;
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

FSM_t fsm = {
		.currentState = INIT,
		.previousState = HALT,
		.stateEntry = 1
};

volatile uint8_t guiCommand = 0;
static SensorData sensorData;
PreRunStatus preRun;

#define THERMISTOR_COUNT 8
uint16_t rawValues[THERMISTOR_COUNT];
float thermistorValues[THERMISTOR_COUNT];
INA219_t ina219_left;
INA219_t ina219_right;
uint32_t object_distance;

#define SENSOR_INIT_DONE (1 << 0)
#define ADC_READY_FLAG (1 << 0)
osMessageQueueId_t commandQueue;
osMutexId_t sensorMutex;
osMutexId_t i2cMutex;
osEventFlagsId_t sensorInitFlag;
osEventFlagsId_t adcFlag;


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();
  MX_TIM1_Init();
  MX_UART7_Init();

  commandQueue = osMessageQueueNew(10, sizeof(uint8_t), NULL);
  if (commandQueue == NULL) Error_Handler();

  sensorMutex = osMutexNew(NULL);
  if (sensorMutex == NULL) Error_Handler();

  i2cMutex = osMutexNew(NULL);
  if (i2cMutex == NULL) Error_Handler();

  /* Init scheduler */
  osKernelInitialize();

  /* Create the thread(s) */
  lidarTaskHandle = osThreadNew(StartLidarTask, &sensorData, &lidarTask_attributes);
  mpuTaskHandle = osThreadNew(StartMPUTask, &sensorData, &mpuTask_attributes);
  thermistorsTaskHandle = osThreadNew(StartThermistorsTask, &sensorData, &thermistorsTask_attributes);
  INATaskHandle = osThreadNew(StartINATask, &sensorData, &INATask_attributes);
  telemetryTaskHandle = osThreadNew(StartTelemetryTask, NULL, &telemetryTask_attributes);
  commandTaskHandle = osThreadNew(StartCommandTask, NULL, &commandTask_attributes);
  fsmTaskHandle = osThreadNew(StartFSMTask, NULL, &fsmTask_attributes);

  /* Initialize leds */
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_BLUE);
  BSP_LED_Init(LED_RED);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Initialize COM1 port (115200, 8 bits (7-bit data + 1 stop bit), no parity */
  BspCOMInit.BaudRate   = 115200;
  BspCOMInit.WordLength = COM_WORDLENGTH_8B;
  BspCOMInit.StopBits   = COM_STOPBITS_1;
  BspCOMInit.Parity     = COM_PARITY_NONE;
  BspCOMInit.HwFlowCtl  = COM_HWCONTROL_NONE;
  if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  /* Start scheduler */
  sensorInitFlag = osEventFlagsNew(NULL);
  if (sensorInitFlag == NULL) Error_Handler();

  adcFlag = osEventFlagsNew(NULL);
  if (adcFlag == NULL) Error_Handler();

  osKernelStart();
}

/* USER CODE BEGIN 4 */
void blink_yellow(void)
{
    static uint32_t lastToggle = 0;
    static uint8_t ledOn = 0;

    if (osKernelGetTickCount() - lastToggle > 500)
    {
        lastToggle = osKernelGetTickCount();
        ledOn = !ledOn;

        if (ledOn)
            WS2812_SetAll(128, 128, 0);
        else
            WS2812_SetAll(0, 0, 0);

        WS2812_Start();
    }
}

void blink_red(void)
{
    static uint32_t lastToggle = 0;
    static uint8_t ledOn = 0;

    if (osKernelGetTickCount() - lastToggle > 500)
    {
        lastToggle = osKernelGetTickCount();
        ledOn = !ledOn;

        if (ledOn)
            WS2812_SetAll(180, 0, 0);
        else
            WS2812_SetAll(0, 0, 0);

        WS2812_Start();
    }
}

void init_sensors(void) {
	// LIDAR
	printf("LIDAR initializing...\r\n");
	lidar_init(&hi2c1);
	osMutexAcquire(i2cMutex, osWaitForever);
	lidar_config(4);
	osMutexRelease(i2cMutex);
	printf("Finished LIDAR initialization.\r\n");

	// MPU
	printf("MPU initializing...\r\n");
	osMutexAcquire(i2cMutex, osWaitForever);
	MPU6050_Initialization(&hi2c1);
	osMutexRelease(i2cMutex);
	printf("Finished MPU initialization.\r\n");

	// THERMISTORS
	printf("THERMS initializing...\r\n");
	hadc1.Init.ContinuousConvMode = ENABLE; // DMA in circular mode
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)rawValues, THERMISTOR_COUNT);
	osEventFlagsSet(adcFlag, ADC_READY_FLAG);
	printf("Finished THERMS initialization.\r\n");

	// LED
	printf("LED initializing...\r\n");
	WS2812_Init(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
	printf("Finished LED initialization.\r\n");

	// INA
	printf("INAs initializing...\r\n");
    if (!INA219_Init(&ina219_left, &hi2c1, INA219_ADDRESS)) {
	    Error_Handler();
    }

    if (!INA219_Init(&ina219_right, &hi2c1, INA219_ADDRESS1)) {
	    Error_Handler();
    }
	printf("Finished INAs initialization.\r\n");


	printf("===== SENSOR INITIALIZATION COMPLETE =====\r\n");
	osEventFlagsSet(sensorInitFlag, SENSOR_INIT_DONE);
}

void pre_run_checklist(SensorData *data)
{
	// testing purposes lolz
//	preRun.sensorsOk = 1;
//	preRun.brakesClosed = 1;
//	preRun.tiltOk = 1;
//	preRun.pneumaticsOk = 1;
//	preRun.batteryOk = 1;

	SensorData localCopy;
	osMutexAcquire(sensorMutex, osWaitForever);
	memcpy(&localCopy, data, sizeof(SensorData));
	osMutexRelease(sensorMutex);


    // TODO: GUI comms check

    // sensors initialized -- can be read
	preRun.sensorsOk = 1;

    // TODO: brakes closed

	// LEDs turned on
	WS2812_SetAll(128, 0, 64);
	WS2812_Start();
	preRun.ledsOk = 1;

    // tilt range check -- roll, pitch
		// TODO: change bounds vals and error ping
	if (localCopy.roll >= 23.34 || localCopy.pitch >= 23.34) {
		preRun.tiltOk = 0;
		printf("ERROR");
	}
	preRun.tiltOk = 1;

    // TODO: pneumatics pressure

    // TODO: battery safe -- within V, I, T range

	preRun.allOk = (
		preRun.sensorsOk &&
		preRun.brakesClosed &&
		preRun.ledsOk &&
		preRun.tiltOk &&
		preRun.pneumaticsOk &&
		preRun.batteryOk
	);
}

bool fault_conditions(SensorData *data) {
	// dynamics
	if (data->roll >= 23.34 || data->pitch >= 23.34) return 1;

	// TODO: braking (pneumatics)

	// LIM
	for (int i = 0; i < THERMISTOR_COUNT; i++) {
		if (data->thermistors[i] >= 80) return 1;
	}

	// TODO: battery

	// TODO: powers

	// TODO: check comms signal

	// LiDAR check
	if (data->lidar_dist > 114) return 1;
	return 0;
}

void init_actions(SensorData *data) {
    printf("Entering INIT state\r\n");

	init_sensors();

    // TODO: brakes CLOSED
//    HAL_GPIO_WritePin(GPIOX, BRAKE_PIN, GPIO_PIN_SET);

    // TODO: establish GUI comms again

    // TODO: LV system ON -- calibrate sensors??? what does this mean
//    HAL_GPIO_WritePin(GPIOX, LV_ENABLE_PIN, GPIO_PIN_SET);

    // TODO: send sensor data to GUI


	pre_run_checklist(&sensorData);
	blink_yellow();
    printf("INIT complete -- waiting for transition\r\n");
}

void load_actions(SensorData *data) {
    printf("Entering LOAD state\r\n");

    // TODO: brakes OPEN
    //HAL_GPIO_WritePin(GPIOX, BRAKE_PIN, GPIO_PIN_RESET);

    // TODO: LV stays ON
//    HAL_GPIO_WritePin(GPIOX, LV_ENABLE_PIN, GPIO_PIN_SET);

    // TODO: HV OFF
//    HAL_GPIO_WritePin(GPIOX, HV_ENABLE_PIN, GPIO_PIN_RESET);

    // TODO: send sensor data to GUI

	blink_yellow();
    printf("LOAD complete -- waiting for transition\r\n");
}

void precharge_actions(SensorData *data) {
	// TODO: brakes open

	// TODO: turn on HV sequence

	// TODO: send sensor data to GUI

	blink_yellow();
	printf("PRECHARGE complete -- waiting for transition\r\n");
}

void start_actions(SensorData *data) {
	//	TODO: OpenBrakes();
	//	TODO: StartVFD_LIM();
	//	TODO: SendSensorDataToGUI(data);

	// solid green
	WS2812_SetAll(0, 180, 0);
	WS2812_Start();
	printf("START complete -- waiting for transition\r\n");
}


void stop_actions(SensorData *data) {
	//	TODO: CloseBrakes();
	//	TODO: SetHVPower(OFF);
	//	TODO: SendSensorDataToGUI(data);


	// solid red
	WS2812_SetAll(180, 0, 0);
	WS2812_Start();
	printf("STOP complete -- waiting for transition\r\n");
}

void fault_actions(SensorData *data) {
	// TODO: CloseBrakes();
	// TODO: EmergencyRelayCutoff();
	// TODO: StoreFaultInfo(data);


	// solid red
	WS2812_SetAll(180, 0, 0);
	WS2812_Start();
	printf("FAULT complete -- waiting for transition\r\n");
}

void halt_actions(SensorData *data) {
	// TODO: CloseBrakes();
	// TODO: SaveToOnboardMemory(data);
	// TODO: SetHVPower(OFF);


	// solid red
	WS2812_SetAll(180, 0, 0);
	WS2812_Start();
	printf("START complete -- waiting for transition\r\n");
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartLidarTask */
/**
  * @brief  Function implementing the lidarTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartLidarTask */
void StartLidarTask(void *argument)
{
  /* USER CODE BEGIN 5 */
//	printf("Lidar Task Starting...\r\n");

	osEventFlagsWait(sensorInitFlag, SENSOR_INIT_DONE, osFlagsNoClear, osWaitForever);
	char uart_tx_buff[100];
	SensorData *data = (SensorData *)argument;

	for(;;)
	{
		osMutexAcquire(i2cMutex, osWaitForever);
		object_distance = retrieve_lidar_distance();
		osMutexRelease(i2cMutex);

		osMutexAcquire(sensorMutex, osWaitForever);
		data->lidar_dist = object_distance;
		osMutexRelease(sensorMutex);

		sprintf(uart_tx_buff, "LIDAR Distance: %lu cm\r\n", object_distance);
		HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)uart_tx_buff, strlen(uart_tx_buff), 100);

		printf("Lidar task ALIVE");
		osDelay(300);
	}
  /* USER CODE END 5 */
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
//	  printf("MPU Task Starting...\r\n");

	  osEventFlagsWait(sensorInitFlag, SENSOR_INIT_DONE, osFlagsNoClear, osWaitForever);
	  float roll = 0, pitch = 0;
	  const float alpha = 0.98f;   // 98% gyro, 2% accelerometer
	  uint32_t lastTick = osKernelGetTickCount();
	  SensorData *data = (SensorData *)argument;
//	  char uart_tx_buff[150];

	  /* Infinite loop */
	  for(;;)
	  {
		  if(MPU6050_DataReady()) {
			  uint32_t now = osKernelGetTickCount();
			  float dt = (now - lastTick) / 1000.0f;
			  lastTick = now;

			  osMutexAcquire(i2cMutex, osWaitForever);
			  MPU6050_ProcessData(&MPU6050);
			  osMutexRelease(i2cMutex);
			  float acc_roll  = atan2(MPU6050.acc_y, MPU6050.acc_z) * 180.0f / M_PI;
			  float acc_pitch = atan2(-MPU6050.acc_x, sqrtf(MPU6050.acc_y*MPU6050.acc_y +MPU6050.acc_z*MPU6050.acc_z))
								  * 180.0f / M_PI;

			  // 2. Integrate gyro
			  roll  += MPU6050.gyro_x * dt;
			  pitch += MPU6050.gyro_y * dt;

			  // 3. Complementary filter
			  roll  = alpha * roll  + (1 - alpha) * acc_roll;
			  pitch = alpha * pitch + (1 - alpha) * acc_pitch;

			  osMutexAcquire(sensorMutex, osWaitForever);
			  data->roll = roll;
			  data->pitch = pitch;
			  osMutexRelease(sensorMutex);

//			  sprintf(uart_tx_buff, "Roll: %f  Pitch: %f\r\n", roll, pitch);
//			  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);
//			  sprintf(uart_tx_buff, "Acc | x: %f, y: %f, z: %f\r\n", MPU6050.acc_x, MPU6050.acc_y, MPU6050.acc_z);
//			  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);
//			  sprintf(uart_tx_buff, "Gyro | x: %f, y: %f, z: %f\r\n", MPU6050.gyro_x, MPU6050.gyro_y, MPU6050.gyro_z);
//			  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);
//			  sprintf(uart_tx_buff, "Acc Raw | x: %d, y: %d, z: %d\r\n", MPU6050.acc_x_raw, MPU6050.acc_y_raw, MPU6050.acc_z_raw);
//			  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);
//
			 }
//	  printf("MPU task ran!\r\n");
	  osDelay(300);
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
	    osEventFlagsWait(adcFlag, ADC_READY_FLAG, osFlagsNoClear, osWaitForever);
	    osEventFlagsWait(sensorInitFlag, SENSOR_INIT_DONE, osFlagsNoClear, osWaitForever);
		uint32_t startTime = HAL_GetTick();
	    uint32_t hours = 0;
	    uint32_t minutes = 0;
	    uint32_t seconds = 0;
	    uint32_t time_elapsed = 0;
	    SensorData *data = (SensorData *)argument;
//	    char uart_tx_buff[100];

//	    hadc1.Init.ContinuousConvMode = ENABLE; // DMA in circular mode
//	  memset(rawValues, 0, sizeof(rawValues));  // clear data

	    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)rawValues, THERMISTOR_COUNT);


	  /* Infinite loop */
	  for(;;)
	  {
		  if (convCompleted) {
			  convCompleted = 0;
			  time_elapsed = HAL_GetTick() - startTime;
			  convert_millis_to_hms(time_elapsed, &hours, &minutes, &seconds);
			  for (int i=0; i<THERMISTOR_COUNT; i++) {
//				  thermistorValues[i] = rawValues[i];
				  float value = ntc_convertToC(rawValues[i]);
//				  float value = 72.01;
				  thermistorValues[i] = value;
				  if (i == 0) {
//					  sprintf(uart_tx_buff, "Thermistor %d,%.2f,%02lu:%02lu:%02lu\r\n", i, value, hours, minutes, seconds);
//					  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);
//					  printf("Thermistor %d,%.2f\r\n", i, value);

				  }
			  }

			  osMutexAcquire(sensorMutex, osWaitForever);
			  memcpy(data->thermistors, thermistorValues, sizeof(data->thermistors));
			  osMutexRelease(sensorMutex);
		  }
//		  printf("Thermistors ALIVE\r\n");
		  osDelay(1000);
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
	  osEventFlagsWait(sensorInitFlag, SENSOR_INIT_DONE, osFlagsNoClear, osWaitForever);
	  SensorData *data = (SensorData *)argument;
	  uint16_t vbus, vshunt, current, power;
	  uint16_t vbus1, vshunt1, current1, power1;
//	  char uart_tx_buff[100];

	  /* Infinite loop */
	  for(;;)
	  {
		  osMutexAcquire(i2cMutex, osWaitForever);
		  vbus = INA219_ReadBusVoltage(&ina219_left);
		  vshunt = INA219_ReadShuntVoltage(&ina219_left);
		  current = INA219_ReadCurrent(&ina219_left);
		  power = INA219_ReadPower(&ina219_left);
		  osMutexRelease(i2cMutex);


//		  sprintf(uart_tx_buff, "vbus: %hu mV\r\n",vbus);
//		  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);
//
//		  sprintf(uart_tx_buff, "vShunt: %hu mV\r\n",vshunt);
//		  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);
//
//		  sprintf(uart_tx_buff, "current: %hu mA\r\n",current);
//		  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);
//
//		  sprintf(uart_tx_buff, "power: %hu mW\r\n",power );
//		  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);
//
		  osMutexAcquire(i2cMutex, osWaitForever);
		  vbus1 = INA219_ReadBusVoltage(&ina219_right);
		  vshunt1 = INA219_ReadShuntVoltage(&ina219_right);
		  current1 = INA219_ReadCurrent(&ina219_right);
		  power1 = INA219_ReadPower(&ina219_right);
		  osMutexRelease(i2cMutex);

		  osMutexAcquire(sensorMutex, osWaitForever);
		  data->pt_up = power;
		  data->pt_down = power1;
		  osMutexRelease(sensorMutex);
//
//		  sprintf(uart_tx_buff, "vbus 1: %hu mV\r\n",vbus1);
//		  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);
//
//		  sprintf(uart_tx_buff, "vShunt 1: %hu mV\r\n",vshunt1);
//		  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);
//
//		  sprintf(uart_tx_buff, "current 1: %hu mA\r\n",current1);
//		  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);
//
//		  sprintf(uart_tx_buff, "power 1: %hu mW\r\n\n",power1);
//		  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);

//		printf("INA Task Alive!\r\n");
//		  sprintf(uart_tx_buff, "INA Task\r\n");
//		  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)uart_tx_buff, strlen(uart_tx_buff), 100);

		osDelay(300);
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
  char uart_tx_buff[100];
  for(;;)
  {

	  sprintf(uart_tx_buff, "Starting telemetry task\r\n");
	  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)uart_tx_buff, strlen(uart_tx_buff), 100);
	  sensorData.start_marker = 0xAA;
	  sensorData.lidar_dist = 13;
	  sensorData.roll = 2;
	  sensorData.pitch = 3;
	  memcpy(sensorData.thermistors, thermistorValues, sizeof(sensorData.thermistors));
	  sensorData.pt_up = 4;
	  sensorData.pt_down = 5;
	  sensorData.lv_batt = 6;
	  sensorData.hv_batt_temp = 7;
	  sensorData.batt_soc = 8;
	  sensorData.lim_volt = 9;
	  sensorData.lim_curr = 10;
	  sensorData.hv_batt = 11;
	  sensorData.imd = 12;
	  sensorData.pod_state = 1;
	  strncpy(sensorData.message, "Whatever message", sizeof(sensorData.message));
	  // DMA problem??
//	  HAL_UART_Transmit_DMA(&huart7, (uint8_t*)&sensorData, sizeof(SensorData));
	  sprintf(uart_tx_buff, "Send telemetry data\r\n");
	  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)uart_tx_buff, strlen(uart_tx_buff), 100);

	  HAL_StatusTypeDef res = HAL_UART_Transmit(&huart7, (uint8_t*)&sensorData, sizeof(SensorData), 100);

	  if (res == HAL_OK) {
	      sprintf(uart_tx_buff, "UART7 Sent %d bytes successfully\r\n", sizeof(SensorData));
	      HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)uart_tx_buff, strlen(uart_tx_buff), 100);
	  } else {
	      sprintf(uart_tx_buff, "UART7 ERROR! Status: %d, Code: %lu\r\n", res, huart7.ErrorCode);
	      HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)uart_tx_buff, strlen(uart_tx_buff), 100);
	  }
	  osDelay(1000);
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
	HAL_UART_Receive_IT(&huart7, rxBuffer, 1);
	char cmd_debug_msg[50];
  for(;;)
  {
	  if (newCommandFlag == 1) {
		  newCommandFlag = 0;
		  u_int8_t recievedCmd = rxBuffer[0]; // pulled command from the queue
		  osMessageQueuePut(commandQueue, &recievedCmd, 0, 0);

		  sprintf(cmd_debug_msg, ">>>>>>>>>>>>>RECEIVED CMD: %d\r\n", rxBuffer[0]);
//		  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)cmd_debug_msg, strlen(cmd_debug_msg), 100);
		  if (huart7.ErrorCode != HAL_UART_ERROR_NONE) {
			  HAL_UART_AbortReceive(&huart7);
		  }
		  HAL_UART_Receive_IT(&huart7, rxBuffer, 1);
	  } else {
//		  printf("No command received....\r\n");
	  }
	  if (huart7.ErrorCode != HAL_UART_ERROR_NONE) {
		  sprintf(cmd_debug_msg, "UART7 Error Detected: %lu. Resetting...\r\n", huart7.ErrorCode);
//		  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)cmd_debug_msg, strlen(cmd_debug_msg), 100);

		  HAL_UART_AbortReceive(&huart7);
		  HAL_UART_Receive_IT(&huart7, rxBuffer, 1);
	  }
    osDelay(500);
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
  /* Infinite loop */
  for(;;)
  {
	    //run_pre_run_checklist(&sensorData);

		SensorData localCopy;

		osMutexAcquire(sensorMutex, osWaitForever);
		memcpy(&localCopy, &sensorData, sizeof(SensorData));
		osMutexRelease(sensorMutex);

		bool auto_trigger = fault_conditions(&localCopy);

		uint8_t pendingCmd;
		bool hasCommand = false; // checks to see if queue is empty

		if (osMessageQueueGet(commandQueue, &pendingCmd, NULL, 0) == osOK) { // reads and pops a cmd from queue
			hasCommand = true;
		}

		switch(fsm.currentState) {
		case INIT:
			if (fsm.stateEntry) {
				init_actions(&sensorData);
				fsm.stateEntry = 0;
			}

			// manual GUI trigger
			if(preRun.allOk && hasCommand && pendingCmd == CMD_LOAD) {
				//guiCommand = CMD_NONE; // freertos message queue FIFO buffer -- race condition
				fsm.previousState = fsm.currentState;
				fsm.currentState = LOAD;
				fsm.stateEntry = 1;
			}
			break;
		case LOAD:
			if (fsm.stateEntry) {
				load_actions(&sensorData);
				fsm.stateEntry = 0;
			}

			// automatic trigger
			fsm.previousState = fsm.currentState;
			if (auto_trigger) { // this state passes through
				fsm.currentState = FAULT;
				fsm.stateEntry = 1;
			} else if(preRun.allOk && hasCommand && pendingCmd == CMD_PRECHARGE) { // manual GUI trigger
				fsm.currentState = PRECHARGE;
				fsm.stateEntry = 1;
			} else if(preRun.allOk && hasCommand && pendingCmd == CMD_STOP) { // manual GUI trigger
				fsm.currentState = STOP;
				fsm.stateEntry = 1;
			} else {
				fsm.currentState = LOAD;
				fsm.stateEntry = 1;
			}
			break;
		case PRECHARGE:
			if (fsm.stateEntry) {
				precharge_actions(&sensorData);
				fsm.stateEntry = 0;
			}

			fsm.previousState = fsm.currentState;
			if (auto_trigger) {
				fsm.currentState = FAULT;
				fsm.stateEntry = 1;
			}
			// TODO: automatic trigger -- if voltage/current stabilize
  //			else if (voltage_current) {
  //				fsm.currentState = START;
  //		        fsm.stateEntry = 1;
  //			}
			else if(hasCommand && pendingCmd == CMD_STOP) { // manual GUI trigger
				fsm.currentState = STOP;
				fsm.stateEntry = 1;
			}
			break;
		case START:
			if (fsm.stateEntry) {
				start_actions(&sensorData);
				fsm.stateEntry = 0;
			}

			fsm.previousState = fsm.currentState;
			if (auto_trigger) {
				fsm.currentState = FAULT;
				fsm.stateEntry = 1;
			} else if(hasCommand && pendingCmd == CMD_STOP) { // manual GUI trigger
				fsm.currentState = STOP;
				fsm.stateEntry = 1;
			}
			break;
		case FAULT:
			if (fsm.stateEntry) {
				fault_actions(&sensorData);
				fsm.stateEntry = 0;
			}

			fsm.previousState = fsm.currentState;
			fsm.currentState = HALT;
			fsm.stateEntry = 1;
			break;
		case HALT:
			if (fsm.stateEntry) {
				halt_actions(&sensorData);
				fsm.stateEntry = 0;
			}

			fsm.previousState = fsm.currentState;
			if (hasCommand && pendingCmd == CMD_INIT) {
				fsm.currentState = INIT;
				fsm.stateEntry = 1;
			}
			break;
		case STOP:
			if (fsm.stateEntry) {
				stop_actions(&sensorData);
				fsm.stateEntry = 0;
			}

			fsm.previousState = fsm.currentState;
			if (auto_trigger) {
				fsm.currentState = FAULT;
				fsm.stateEntry = 1;
			}
			else if (hasCommand && pendingCmd == CMD_INIT) {
				fsm.currentState = INIT;
				fsm.stateEntry = 1;
			} else if (hasCommand && pendingCmd == CMD_LOAD) {
				fsm.currentState = LOAD;
				fsm.stateEntry = 1;
			}
			break;
		}
    osDelay(50);
  }
  /* USER CODE END StartFSMTask */
}


/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM2 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM2)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
