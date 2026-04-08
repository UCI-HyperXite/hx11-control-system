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

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
int retrieve_lidar_distance();
void lidar_config(int);

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LIDAR_ADD 0x62<<1  // i2c slave address of lidar lite
I2C_HandleTypeDef *LIDAR_I2C_Handler;


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
uint32_t m_distance,object_distance;
uint8_t cmd[1];
uint8_t data[2]={10};


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
void convert_millis_to_hms(uint32_t, uint32_t*, uint32_t*, uint32_t*);
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
uint8_t newCommandFlag = 0;
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

osMessageQueueId_t commandQueue;
osMutexId_t sensorMutex;
osMutexId_t i2cMutex;

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
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  commandQueue = osMessageQueueNew(10, sizeof(uint8_t), NULL);
  sensorMutex = osMutexNew(NULL);
  i2cMutex = osMutexNew(NULL);
  /* Init scheduler */
  osKernelInitialize();

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
  /* creation of lidarTask */
  lidarTaskHandle = osThreadNew(StartLidarTask, &sensorData, &lidarTask_attributes);

  /* creation of mpuTask */
  mpuTaskHandle = osThreadNew(StartMPUTask, &sensorData, &mpuTask_attributes);

  /* creation of thermistorsTask */
  thermistorsTaskHandle = osThreadNew(StartThermistorsTask, &sensorData, &thermistorsTask_attributes);

  /* creation of INATask */
  INATaskHandle = osThreadNew(StartINATask, &sensorData, &INATask_attributes);

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
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

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
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
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

void lidar_init(I2C_HandleTypeDef *hi2c)
{
    LIDAR_I2C_Handler = hi2c;
}

void lidar_config(int configur)
{
    // configuration setting for lidar
    cmd[0] = 0x04;
    osMutexAcquire(i2cMutex, osWaitForever);
    HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x00,1,cmd,1,0x100);
    switch(configur)
    {
    case 0:
        //default mode , balance mode
        cmd[0]=0x80;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x02,1,cmd,1,0x1000);
        cmd[0]=0x04;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x04,1,cmd,1,0x1000);
        cmd[0]=0x00;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x1c,1,cmd,1,0x1000);
        break;

    case 1:
        //short range, high speed
        cmd[0]=0x1d;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x02,1,cmd,1,0x1000);
        cmd[0]=0x08;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x04,1,cmd,1,0x1000);
        cmd[0]=0x00;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x1c,1,cmd,1,0x1000);
        break;

    case 2:
        //default range, higher speed short range
        cmd[0]=0x80;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x02,1,cmd,1,0x1000);
        cmd[0]=0x08;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x04,1,cmd,1,0x1000);
        cmd[0]=0x00;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x1c,1,cmd,1,0x1000);
        break;

    case 3:
        //maximum Range
        cmd[0]=0xff;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x02,1,cmd,1,0x1000);
        cmd[0]=0x08;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x04,1,cmd,1,0x1000);
        cmd[0]=0x00;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x1c,1,cmd,1,0x1000);
        break;

    case 4:
        //high sensitivity detection, high  measurement
        cmd[0]=0x80;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x02,1,cmd,1,0x1000);
        cmd[0]=0x08;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x04,1,cmd,1,0x1000);
        cmd[0]=0x80;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x1c,1,cmd,1,0x1000);
        break;

    case 5:
        //low sensitivity detection , low  measurement
        cmd[0]=0x80;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x02,1,cmd,1,0x1000);
        cmd[0]=0x08;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x04,1,cmd,1,0x1000);
        cmd[0]=0xb0;
        HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x1c,1,cmd,1,0x1000);
        break;
    }
    osMutexRelease(i2cMutex);
}


int retrieve_lidar_distance()
{
	osMutexAcquire(i2cMutex, osWaitForever);
    cmd[0]=0x04;
    HAL_I2C_Mem_Write(LIDAR_I2C_Handler,LIDAR_ADD ,0x00,1,cmd,1,100);
    cmd[0]=0x8f;
    HAL_I2C_Master_Transmit(LIDAR_I2C_Handler,LIDAR_ADD,cmd,1,100);
    HAL_I2C_Master_Receive(LIDAR_I2C_Handler,LIDAR_ADD,data,2,100);
    m_distance = (data[0]<<8)|(data[1]);
    osMutexRelease(i2cMutex);
    return m_distance ;
}

void convert_millis_to_hms(uint32_t total_milliseconds, uint32_t* hours, uint32_t* minutes, uint32_t* seconds) {
    uint32_t total_seconds = total_milliseconds / 1000;

    *seconds = total_seconds % 60;
    *minutes = (total_seconds / 60) % 60;
    *hours = total_seconds / 3600;
}

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
	lidar_config(4);
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
	printf("Finished THERMS initialization.\r\n");

	// LED
	printf("LED initializing...\r\n");
	WS2812_Init(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
	printf("Finished LED initialization.\r\n");

	// INA
	printf("INAs initializing...\r\n");
<<<<<<< HEAD
	osMutexAcquire(i2cMutex, osWaitForever);
    if (!INA219_Init(&ina219_left, &hi2c1, INA219_ADDRESS)) {
	    Error_Handler();
    }

    if (!INA219_Init(&ina219_right, &hi2c1, INA219_ADDRESS1)) {
	    Error_Handler();
    }
	osMutexRelease(i2cMutex);
=======
//    if (!INA219_Init(&ina219_left, &hi2c1, INA219_ADDRESS)) {
//	    Error_Handler();
//    }
//
//    if (!INA219_Init(&ina219_right, &hi2c1, INA219_ADDRESS1)) {
//	    Error_Handler();
//    }
>>>>>>> branch 'feature/FreeRTOS-refactor' of https://github.com/UCI-HyperXite/hx11-control-system.git
	printf("Finished INAs initialization.\r\n");
	printf("===== SENSOR INITIALIZATION COMPLETE =====\r\n");
}

void pre_run_checklist(SensorData *data)
{
	// testing purposes lolz
//	preRun.sensorsOk = 1;
//	preRun.brakesClosed = 1;
//	preRun.tiltOk = 1;
//	preRun.pneumaticsOk = 1;
//	preRun.batteryOk = 1;


    // TODO: GUI comms check

    // sensors initialized -- can be read
	init_sensors();
	preRun.sensorsOk = 1;

    // TODO: brakes closed

	// LEDs turned on
	WS2812_SetAll(128, 0, 64);
	WS2812_Start();
	preRun.ledsOk = 1;

    // tilt range check -- roll, pitch
		// TODO: change bounds vals and error ping
	if (data->roll >= 23.34 || data->pitch >= 23.34) {
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
//
//	/* Configure lidar once */
//	lidar_init(&hi2c1);
//	lidar_config(4);
////	uint32_t object_distance;
//
	char uart_tx_buff[100];
	SensorData *data = (SensorData *)argument;

	for(;;)
	{
		object_distance = retrieve_lidar_distance();
		data->lidar_dist = object_distance;
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

	  // MPU initialization
//	  MPU6050_Initialization(&hi2c1);
//	  printf("Finished MPU initialization.\r\n");
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

			  data->roll = roll;
			  data->pitch = pitch;

//			  sprintf(uart_tx_buff, "Roll: %f  Pitch: %f\r\n", roll, pitch);
//			  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);
//			  sprintf(uart_tx_buff, "Acc | x: %f, y: %f, z: %f\r\n", MPU6050.acc_x, MPU6050.acc_y, MPU6050.acc_z);
//			  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);
//			  sprintf(uart_tx_buff, "Gyro | x: %f, y: %f, z: %f\r\n", MPU6050.gyro_x, MPU6050.gyro_y, MPU6050.gyro_z);
//			  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);
//			  sprintf(uart_tx_buff, "Acc Raw | x: %d, y: %d, z: %d\r\n", MPU6050.acc_x_raw, MPU6050.acc_y_raw, MPU6050.acc_z_raw);
//			  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);
<<<<<<< HEAD
			 }
	  printf("MPU task ran!\r\n");
=======
//
//			 }
//	  printf("MPU task ran!\r\n");
>>>>>>> branch 'feature/FreeRTOS-refactor' of https://github.com/UCI-HyperXite/hx11-control-system.git
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
		uint32_t startTime = HAL_GetTick();
	    uint32_t hours = 0;
	    uint32_t minutes = 0;
	    uint32_t seconds = 0;
	    uint32_t time_elapsed = 0;
	    SensorData *data = (SensorData *)argument;
//	    char uart_tx_buff[100];

	    hadc1.Init.ContinuousConvMode = ENABLE; // DMA in circular mode
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
			  memcpy(data->thermistors, thermistorValues, sizeof(data->thermistors));
//			  // Use RTOS atomic operation for flag
//			  taskENTER_CRITICAL();
//			  thermistorDataReady = 1;
//			  taskEXIT_CRITICAL();
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
<<<<<<< HEAD
	  INA219_t ina219;
	  uint16_t vbus, vshunt, current, power;

	  INA219_t ina219_1;
	  uint16_t vbus1, vshunt1, current1, power1;
	  SensorData *data = (SensorData *)argument;
=======
//	  INA219_t ina219;
//	  uint16_t vbus, vshunt, current, power;
//
//	  INA219_t ina219_1;
//	  uint16_t vbus1, vshunt1, current1, power1;
>>>>>>> branch 'feature/FreeRTOS-refactor' of https://github.com/UCI-HyperXite/hx11-control-system.git
//	  char uart_tx_buff[100];
//	  if (!INA219_Init(&ina219, &hi2c1, INA219_ADDRESS)) {
//		  Error_Handler();
//	  }
//
//	  if (!INA219_Init(&ina219_1, &hi2c1, INA219_ADDRESS1)) {
//		  Error_Handler();
//	  }

	  /* Infinite loop */
	  for(;;)
	  {
		  osMutexAcquire(i2cMutex, osWaitForever);
		  vbus = INA219_ReadBusVoltage(&ina219);
		  vshunt = INA219_ReadShuntVoltage(&ina219);
		  current = INA219_ReadCurrent(&ina219);
		  power = INA219_ReadPower(&ina219);
		  osMutexRelease(i2cMutex);
		  data->pt_up = power;

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
		  vbus1 = INA219_ReadBusVoltage(&ina219_1);
		  vshunt1 = INA219_ReadShuntVoltage(&ina219_1);
		  current1 = INA219_ReadCurrent(&ina219_1);
		  power1 = INA219_ReadPower(&ina219_1);
		  osMutexRelease(i2cMutex);
		  data->pt_down = power1;
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
		bool auto_trigger = fault_conditions(&sensorData);

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
			} else if(preRun.allOk && hasCommand && pendingCmd == CMD_PRECHARGE) { // manual GUI trigger
				fsm.currentState = PRECHARGE;
			} else if(preRun.allOk && hasCommand && pendingCmd == CMD_STOP) { // manual GUI trigger
				fsm.currentState = STOP;
			} else {
				fsm.currentState = LOAD;
			}
			fsm.stateEntry = 1;
			break;
		case PRECHARGE:
			if (fsm.stateEntry) {
				precharge_actions(&sensorData);
				fsm.stateEntry = 0;
			}

			fsm.previousState = fsm.currentState;
			if (auto_trigger) {
				fsm.currentState = FAULT;
			}
			// TODO: automatic trigger -- if voltage/current stabilize
  //			else if (voltage_current) {
  //				fsm.currentState = START;
  //			}
			else if(hasCommand && pendingCmd == CMD_STOP) { // manual GUI trigger
				guiCommand = CMD_NONE;
				fsm.currentState = STOP;
			}
			fsm.stateEntry = 1;
			break;
		case START:
			if (fsm.stateEntry) {
				start_actions(&sensorData);
				fsm.stateEntry = 0;
			}

			fsm.previousState = fsm.currentState;
			if (auto_trigger) {
				fsm.currentState = FAULT;
			} else if(hasCommand && pendingCmd == CMD_STOP) { // manual GUI trigger
				guiCommand = CMD_NONE;
				fsm.currentState = STOP;
			}
			fsm.stateEntry = 1;
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
			}
			fsm.stateEntry = 1;
			break;
		case STOP:
			if (fsm.stateEntry) {
				stop_actions(&sensorData);
				fsm.stateEntry = 0;
			}

			fsm.previousState = fsm.currentState;
			if (auto_trigger) {
				fsm.currentState = FAULT;
			}
			else if (hasCommand && pendingCmd == CMD_INIT) {
				fsm.currentState = INIT;
			} else if (hasCommand && pendingCmd == CMD_LOAD) {
				fsm.currentState = LOAD;
			}
			fsm.stateEntry = 1;
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
