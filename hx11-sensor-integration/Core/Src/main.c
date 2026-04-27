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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
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

#include "time_utils.h"

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

COM_InitTypeDef BspCOMInit;
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;
DMA_HandleTypeDef hdma_tim1_ch1;

UART_HandleTypeDef huart7;

/* Definitions for lidarTask */
osThreadId_t lidarTaskHandle;
const osThreadAttr_t lidarTask_attributes = {
  .name = "lidarTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal4,
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
  .stack_size = 512 * 4,
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
  .priority = (osPriority_t) osPriorityNormal2,
};
/* Definitions for commandTask */
osThreadId_t commandTaskHandle;
const osThreadAttr_t commandTask_attributes = {
  .name = "commandTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal5,
};
/* Definitions for fsmTask */
osThreadId_t fsmTaskHandle;
const osThreadAttr_t fsmTask_attributes = {
  .name = "fsmTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM1_Init(void);
static void MX_UART7_Init(void);
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

uint8_t lidar_ok = 0;
uint8_t mpu_ok = 0;
uint8_t ina_left_ok = 0;
uint8_t ina_right_ok = 0;

#define SENSOR_INIT_DONE (1 << 0)
#define ADC_READY_FLAG (1 << 0)
osMessageQueueId_t commandQueue;
osMutexId_t sensorMutex;
osMutexId_t i2cMutex;
osEventFlagsId_t sensorInitFlag;
osEventFlagsId_t adcFlag;

void I2C1_BusRecovery(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Temporarily configure SCL (PB6) and SDA (PB9) as GPIO */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    HAL_I2C_DeInit(&hi2c1);

    GPIO_InitStruct.Pin = GPIO_PIN_6;  /* SCL */
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_9;  /* SDA */
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Clock SCL 9 times to release any stuck slave */
    for (int i = 0; i < 9; i++) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
        HAL_Delay(1);

        /* If SDA is high, the bus is free */
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_9) == GPIO_PIN_SET) {
            break;
        }
    }

    /* Generate STOP: SDA low while SCL high, then SDA high */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);
    HAL_Delay(1);

    /* Re-init I2C peripheral */
    MX_I2C1_Init();

    printf("I2C1 bus recovery complete\r\n");
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */

int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();
  MX_TIM1_Init();
  MX_UART7_Init();

  __HAL_RCC_GPIOE_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);


  /* USER CODE BEGIN 2 */

  I2C1_BusRecovery();

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* Create RTOS objects AFTER kernel init */
  commandQueue = osMessageQueueNew(10, sizeof(uint8_t), NULL);
  if (commandQueue == NULL) Error_Handler();

  sensorMutex = osMutexNew(NULL);
  if (sensorMutex == NULL) Error_Handler();

  i2cMutex = osMutexNew(NULL);
  if (i2cMutex == NULL) Error_Handler();

  sensorInitFlag = osEventFlagsNew(NULL);
  if (sensorInitFlag == NULL) Error_Handler();

  adcFlag = osEventFlagsNew(NULL);
  if (adcFlag == NULL) Error_Handler();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
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
//  lidarTaskHandle = osThreadNew(StartLidarTask, &sensorData, &lidarTask_attributes);
//  mpuTaskHandle = osThreadNew(StartMPUTask, &sensorData, &mpuTask_attributes);
  thermistorsTaskHandle = osThreadNew(StartThermistorsTask, &sensorData, &thermistorsTask_attributes);
//  INATaskHandle = osThreadNew(StartINATask, &sensorData, &INATask_attributes);
  telemetryTaskHandle = osThreadNew(StartTelemetryTask, NULL, &telemetryTask_attributes);
  commandTaskHandle = osThreadNew(StartCommandTask, NULL, &commandTask_attributes);
//  fsmTaskHandle = osThreadNew(StartFSMTask, NULL, &fsmTask_attributes);

  init_sensors();

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Initialize leds */
//  BSP_LED_Init(LED_GREEN);
//  BSP_LED_Init(LED_BLUE);
//  BSP_LED_Init(LED_RED);
//
//  BSP_LED_On(LED_GREEN);
//  BSP_LED_On(LED_BLUE);
//  BSP_LED_On(LED_RED);

//  	  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_SET);
//      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
//      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
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
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */



  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}



/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Macro to configure the PLL clock source
  */
  __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSI);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_16B;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  //hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.NbrOfConversion = 8;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
  hadc1.Init.OversamplingMode = DISABLE;
  hadc1.Init.Oversampling.Ratio = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_16;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_810CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  sConfig.OffsetSignedSaturation = DISABLE;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_15;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_18;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_10;
  sConfig.Rank = ADC_REGULAR_RANK_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_11;
  sConfig.Rank = ADC_REGULAR_RANK_6;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_7;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_6;
  sConfig.Rank = ADC_REGULAR_RANK_8;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x10707DBC;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 80-1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief UART7 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART7_Init(void)
{

  /* USER CODE BEGIN UART7_Init 0 */

  /* USER CODE END UART7_Init 0 */

  /* USER CODE BEGIN UART7_Init 1 */

  /* USER CODE END UART7_Init 1 */
  huart7.Instance = UART7;
  huart7.Init.BaudRate = 115200;
  huart7.Init.WordLength = UART_WORDLENGTH_8B;
  huart7.Init.StopBits = UART_STOPBITS_1;
  huart7.Init.Parity = UART_PARITY_NONE;
  huart7.Init.Mode = UART_MODE_TX_RX;
  huart7.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart7.Init.OverSampling = UART_OVERSAMPLING_16;
  huart7.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart7.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart7.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart7) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart7, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart7, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart7) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART7_Init 2 */

  /* USER CODE END UART7_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
  /* DMA1_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : PC8 */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void init_sensors(void) {
//  printf("===== I2C1 BUS SCAN =====\r\n");
//  uint8_t found = 0;
//  for (uint8_t addr = 1; addr < 128; addr++) {
//    if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 10) == HAL_OK) {
//      printf("  0x%02X ACK\r\n", addr);
//      found++;
//    }
//  }
//  if (found == 0) {
//    printf("  NO DEVICES FOUNDfsdsdfsdffsdsdfSDA/SCL wiring order\r\n");
//  }
//  printf("  %d device(s) on bus\r\n", found);
//  printf("=========================\r\n");
//
////  // LIDAR
//  printf("LIDAR initializing...\r\n");
//  lidar_init(&hi2c1);
//  osMutexAcquire(i2cMutex, osWaitForever);
//  lidar_config(4);
//  osMutexRelease(i2cMutex);
//  osMutexAcquire(i2cMutex, osWaitForever);
//  lidar_ok = (HAL_I2C_IsDeviceReady(&hi2c1, 0x62 << 1, 2, 50) == HAL_OK) ? 1 : 0;
//  osMutexRelease(i2cMutex);
//  if (lidar_ok) {
//    printf("Finished LIDAR initialization.\r\n");
//  } else {
//    printf("WARNING: LIDAR not responding on I2C bus\r\n");
//  }
//
//  // MPU
//  printf("MPU initializing...\r\n");
//  osMutexAcquire(i2cMutex, osWaitForever);
//  mpu_ok = (HAL_I2C_IsDeviceReady(&hi2c1, 0xD0, 2, 50) == HAL_OK) ? 1 : 0;
//
//  if (mpu_ok) {
//	  MPU6050_Initialization(&hi2c1);
//      printf("Finished MPU initialization.\r\n");
//    } else {
//      printf("WARNING: MPU6050 not responding on I2C bus\r\n");
//    }
//  osMutexRelease(i2cMutex);


	// THERMISTORS
  printf("THERMS initializing...\r\n");
  //HAL_ADC_Start_DMA(&hadc1, (uint32_t*)rawValues, THERMISTOR_COUNT);
  if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)rawValues, THERMISTOR_COUNT) != HAL_OK) {
	  printf("❌ ADC DMA failed to start\r\n");
  } else {
	  printf("✅ ADC DMA started\r\n");
  }
  osEventFlagsSet(adcFlag, ADC_READY_FLAG);
  printf("Finished THERMS initialization.\r\n");


    // LED
//    printf("LED initializing...\r\n");
//    WS2812_Init(&htim1, TIM_CHANNEL_1);
//    HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
//    WS2812_SetAll(255, 0, 0);
//    WS2812_Start();
//    HAL_Delay(1000);
//    printf("Finished LED initialization.\r\n");


//
//  printf("INAs initializing...\r\n");
//  ina_left_ok = INA219_Init(&ina219_left, &hi2c1, INA219_ADDRESS);
//  if (!ina_left_ok) {
//    printf("WARNING: INA219 LEFT (0x%02X) init failed\r\n", INA219_ADDRESS);
//  }
//
//  ina_right_ok = INA219_Init(&ina219_right, &hi2c1, INA219_ADDRESS1);
//  if (!ina_right_ok) {
//    printf("WARNING: INA219 RIGHT (0x%02X) init failed\r\n", INA219_ADDRESS1);
//  }
//  printf("Finished INAs initialization.\r\n");


  printf("===== SENSOR INITIALIZATION COMPLETE =====\r\n");
  printf("  LIDAR: %s | MPU: %s | INA_L: %s | INA_R: %s\r\n",
    lidar_ok  ? "OK" : "FAIL",
    mpu_ok    ? "OK" : "FAIL",
    ina_left_ok  ? "OK" : "FAIL",
    ina_right_ok ? "OK" : "FAIL");

  osEventFlagsSet(sensorInitFlag, SENSOR_INIT_DONE);
}


void pre_run_checklist(SensorData *data)
{
	SensorData localCopy;
	osMutexAcquire(sensorMutex, osWaitForever);
	memcpy(&localCopy, data, sizeof(SensorData));
	osMutexRelease(sensorMutex);


    // TODO: GUI comms check

    // sensors initialized -- can be read
	preRun.sensorsOk = (lidar_ok && mpu_ok && ina_left_ok && ina_right_ok);

    // TODO: brakes closed

	// LEDs turned on
	WS2812_SetAll(128, 0, 64);
	WS2812_Start();
	preRun.ledsOk = 1;

    // tilt range check -- roll, pitch
	if (localCopy.roll >= 23.34 || localCopy.pitch >= 23.34) {
		preRun.tiltOk = 0;
		printf("ERROR: tilt out of range\r\n");
	} else {
		preRun.tiltOk = 1;
	}

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

	preRun.allOk = 1;
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

	pre_run_checklist(&sensorData);
    printf("INIT complete -- waiting for transition\r\n");
}

void load_actions(SensorData *data) {
    printf("Entering LOAD state\r\n");

    printf("LOAD complete -- waiting for transition\r\n");
}

void precharge_actions(SensorData *data) {
	printf("PRECHARGE complete -- waiting for transition\r\n");
}

void start_actions(SensorData *data) {
	// solid green
	printf("START complete -- waiting for transition\r\n");
}


void stop_actions(SensorData *data) {
	// solid red
	printf("STOP complete -- waiting for transition\r\n");
}

void fault_actions(SensorData *data) {
	// solid red
	printf("FAULT complete -- waiting for transition\r\n");
}

void halt_actions(SensorData *data) {
	// solid red
	printf("HALT complete -- waiting for transition\r\n");
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

	osEventFlagsWait(sensorInitFlag, SENSOR_INIT_DONE, osFlagsNoClear, osWaitForever);

	if (!lidar_ok) {
		printf("LiDAR task: sensor not available, suspending\r\n");
		osThreadSuspend(osThreadGetId());
	}

	char uart_tx_buff[100];
	SensorData *data = (SensorData *)argument;

	for(;;)
	{
		//printf("flag=%d\r\n", convCompleted);
		osMutexAcquire(i2cMutex, osWaitForever);
		object_distance = retrieve_lidar_distance();
		osMutexRelease(i2cMutex);

		osMutexAcquire(sensorMutex, osWaitForever);
		data->lidar_dist = object_distance;
		osMutexRelease(sensorMutex);

		sprintf(uart_tx_buff, "LIDAR Distance: %lu cm\r\n", object_distance);
		HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)uart_tx_buff, strlen(uart_tx_buff), 100);

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

	  osEventFlagsWait(sensorInitFlag, SENSOR_INIT_DONE, osFlagsNoClear, osWaitForever);

	  if (!mpu_ok) {
		  printf("MPU task: sensor not available, suspending\r\n");
		  osThreadSuspend(osThreadGetId());
	  }

	  float roll = 0, pitch = 0;
	  const float alpha = 0.98f;
	  uint32_t lastTick = osKernelGetTickCount();
	  SensorData *data = (SensorData *)argument;

	  char uart_tx_buff[150];

	  /* Infinite loop */
	  for(;;)
	  {
		  /* DataReady() does an I2C read (MPU6050_INT_STATUS register),
		   * so it MUST be inside the i2cMutex. Previously it was unprotected,
		   * which caused bus collisions with LiDAR/INA tasks on the same I2C1. */
		  osMutexAcquire(i2cMutex, osWaitForever);
		  int ready = MPU6050_DataReady();
		  if (ready) {
			  MPU6050_ProcessData(&MPU6050);
		  }
		  osMutexRelease(i2cMutex);

		  if (ready) {
			  uint32_t now = osKernelGetTickCount();
			  float dt = (now - lastTick) / 1000.0f;
			  lastTick = now;

			  float acc_roll  = atan2(MPU6050.acc_y, MPU6050.acc_z) * 180.0f / M_PI;
			  float acc_pitch = atan2(-MPU6050.acc_x, sqrtf(MPU6050.acc_y*MPU6050.acc_y + MPU6050.acc_z*MPU6050.acc_z))
								  * 180.0f / M_PI;

			  roll  += MPU6050.gyro_x * dt;
			  pitch += MPU6050.gyro_y * dt;

			  roll  = alpha * roll  + (1 - alpha) * acc_roll;
			  pitch = alpha * pitch + (1 - alpha) * acc_pitch;

			  sprintf(uart_tx_buff, "Roll: %f  Pitch: %f\r\n", roll, pitch);
			  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);

			  osMutexAcquire(sensorMutex, osWaitForever);
			  data->roll = roll;
			  data->pitch = pitch;
			  osMutexRelease(sensorMutex);
		  } else {
			  /* Keep lastTick current even when no data is ready.
			   * Without this, skipped cycles accumulate into a huge dt
			   * on the next successful read, causing the gyro integration
			   * to overshoot wildly (the -111 / 90 you were seeing). */
			  lastTick = osKernelGetTickCount();
		  }
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

	    // HAL_ADC_Start_DMA(&hadc1, (uint32_t*)rawValues, THERMISTOR_COUNT);


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

	  if (!ina_left_ok && !ina_right_ok) {
		  printf("INA task: no sensors available, suspending\r\n");
		  osThreadSuspend(osThreadGetId());
	  }

	  SensorData *data = (SensorData *)argument;
	  uint16_t power_left = 0, power_right = 0;

	  /* Infinite loop */
	  for(;;)
	  {
		  if (ina_left_ok) {
			  osMutexAcquire(i2cMutex, osWaitForever);
			  power_left = INA219_ReadPower(&ina219_left);
			  osMutexRelease(i2cMutex);
		  }

		  if (ina_right_ok) {
			  osMutexAcquire(i2cMutex, osWaitForever);
			  power_right = INA219_ReadPower(&ina219_right);
			  osMutexRelease(i2cMutex);
		  }

		  osMutexAcquire(sensorMutex, osWaitForever);
		  data->pt_up = power_left;
		  data->pt_down = power_right;
		  osMutexRelease(sensorMutex);

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
  osEventFlagsWait(sensorInitFlag, SENSOR_INIT_DONE, osFlagsNoClear, osWaitForever);

  char json_buf[512];
  SensorData localCopy;

  for(;;)
  {
      osMutexAcquire(sensorMutex, osWaitForever);
      memcpy(&localCopy, &sensorData, sizeof(SensorData));
      osMutexRelease(sensorMutex);

      localCopy.pod_state = (uint8_t)fsm.currentState;

      int len = snprintf(json_buf, sizeof(json_buf),
          "{\"lidar\":%lu,\"pod_state\":%u,"
          "\"roll\":%.2f,\"pitch\":%.2f,"
          "\"therms\":[%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f],"
          "\"pt_up\":%.2f,\"pt_down\":%.2f,"
          "\"lv_batt\":%.2f,\"hv_batt_temp\":%.2f,"
          "\"hv_batt\":%.2f,\"batt_soc\":%.2f,"
          "\"lim_volt\":%.2f,\"lim_curr\":%.2f,"
          "\"imd\":%.2f,\"msg\":\"%s\"}\n",
          (unsigned long)localCopy.lidar_dist,
          localCopy.pod_state,
          localCopy.roll, localCopy.pitch,
          localCopy.thermistors[0], localCopy.thermistors[1],
          localCopy.thermistors[2], localCopy.thermistors[3],
          localCopy.thermistors[4], localCopy.thermistors[5],
          localCopy.thermistors[6], localCopy.thermistors[7],
          localCopy.pt_up, localCopy.pt_down,
          localCopy.lv_batt, localCopy.hv_batt_temp,
          localCopy.hv_batt, localCopy.batt_soc,
          localCopy.lim_volt, localCopy.lim_curr,
          localCopy.imd, "OK");

      HAL_UART_Transmit(&huart7, (uint8_t*)json_buf, len, 200);

      printf("Sent %d bytes JSON to UART7\r\n", len);

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
		  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)cmd_debug_msg, strlen(cmd_debug_msg), 100);
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
const char* state_to_string(pod_status s) {
	switch(s) {
		case INIT: return "INIT";
		case LOAD: return "LOAD";
		case PRECHARGE: return "PRECHARGE";
		case START: return "START";
		case STOP: return "STOP";
		case FAULT: return "FAULT";
		case HALT: return "HALT";
		default: return "UNKNOWN"; }
}

void StartFSMTask(void *argument)
{
  /* USER CODE BEGIN StartFSMTask */
  /* Infinite loop */
	preRun.allOk = 1;
  for(;;)
  {
		uint32_t now = osKernelGetTickCount();
		SensorData localCopy;

		osMutexAcquire(sensorMutex, osWaitForever);
		memcpy(&localCopy, &sensorData, sizeof(SensorData));
		osMutexRelease(sensorMutex);

		//bool auto_trigger = fault_conditions(&localCopy);
		bool auto_trigger = false;

//		uint8_t pendingCmd;
//		bool hasCommand = false;
//
//		if (osMessageQueueGet(commandQueue, &pendingCmd, NULL, 0) == osOK) {
//			hasCommand = true;
//		}

		static uint8_t pendingCmd = CMD_NONE;
		static bool hasCommand = false;


		switch(fsm.currentState) {
		case INIT:
			if (fsm.stateEntry) {
				init_actions(&sensorData);
				fsm.stateEntry = 0;
			}

			if(preRun.allOk && hasCommand && pendingCmd == CMD_LOAD) {
				fsm.previousState = fsm.currentState;
				fsm.currentState = LOAD;
				fsm.stateEntry = 1;
				hasCommand = false;
			}
			break;
		case LOAD:
			if (fsm.stateEntry) {
				load_actions(&sensorData);
				fsm.stateEntry = 0;
			}

			fsm.previousState = fsm.currentState;
			if (auto_trigger) {
				fsm.currentState = FAULT;
				fsm.stateEntry = 1;
				hasCommand = false;
			} else if(preRun.allOk && hasCommand && pendingCmd == CMD_PRECHARGE) {
				fsm.currentState = PRECHARGE;
				fsm.stateEntry = 1;
				hasCommand = false;
			} else if(preRun.allOk && hasCommand && pendingCmd == CMD_STOP) {
				fsm.currentState = STOP;
				fsm.stateEntry = 1;
				hasCommand = false;
			}
			/* no else -- stay in LOAD with stateEntry = 0 */
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
				hasCommand = false;
			}
			else if(hasCommand && pendingCmd == CMD_STOP) {
				fsm.currentState = STOP;
				fsm.stateEntry = 1;
				hasCommand = false;
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
				hasCommand = false;
			} else if(hasCommand && pendingCmd == CMD_STOP) {
				fsm.currentState = STOP;
				fsm.stateEntry = 1;
				hasCommand = false;
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
				hasCommand = false;
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
				hasCommand = false;
			} else if (hasCommand && pendingCmd == CMD_LOAD) {
				fsm.currentState = LOAD;
				fsm.stateEntry = 1;
				hasCommand = false;
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
