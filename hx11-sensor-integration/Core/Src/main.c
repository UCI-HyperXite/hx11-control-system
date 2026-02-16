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


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

COM_InitTypeDef BspCOMInit;
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c3;
I2C_HandleTypeDef hi2c4;

TIM_HandleTypeDef htim1;
DMA_HandleTypeDef hdma_tim1_ch1;

/* Definitions for lidarTask */
osThreadId_t lidarTaskHandle;
const osThreadAttr_t lidarTask_attributes = {
  .name = "lidarTask",
  .stack_size = 762 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for mpuTask */
osThreadId_t mpuTaskHandle;
const osThreadAttr_t mpuTask_attributes = {
  .name = "mpuTask",
  .stack_size = 762 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for thermistorsTask */
osThreadId_t thermistorsTaskHandle;
const osThreadAttr_t thermistorsTask_attributes = {
  .name = "thermistorsTask",
  .stack_size = 762 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
};
/* Definitions for LEDTask */
osThreadId_t LEDTaskHandle;
const osThreadAttr_t LEDTask_attributes = {
  .name = "LEDTask",
  .stack_size = 762 * 4,
  .priority = (osPriority_t) osPriorityNormal2,
};
/* Definitions for INATask */
osThreadId_t INATaskHandle;
const osThreadAttr_t INATask_attributes = {
  .name = "INATask",
  .stack_size = 762 * 4,
  .priority = (osPriority_t) osPriorityNormal3,
};
/* Definitions for FSMTask */
osThreadId_t FSMTaskHandle;
const osThreadAttr_t FSMTask_attributes = {
  .name = "FSMTask",
  .stack_size = 762 * 4,
  .priority = (osPriority_t) osPriorityNormal3,
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
static void MX_I2C3_Init(void);
static void MX_I2C4_Init(void);
static void MX_TIM1_Init(void);
void StartLidarTask(void *argument);
void StartMPUTask(void *argument);
void StartThermistorsTask(void *argument);
void StartLEDTask(void *argument);
void StartINATask(void *argument);
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

typedef struct {
	unint32_t lidar_dist; //LiDAR

	float roll, pitch; //MPU

	float thermistors[8]; //thermies

	float pt_up, pt_down; //ina219

	float lv_batt; //ina260

	float hv_batt_temp, hv_batt; //BMS

	enum pod_status;
} SensorData;


#define EVT_LOAD_COMPLETE       (1 << 0)
#define EVT_PRECHARGE_COMPLETE  (1 << 1)
#define EVT_START_COMPLETE      (1 << 2)
#define EVT_STOP_COMPLETE       (1 << 3)
#define EVT_FAULT               (1 << 4)

enum pod_status {
	INIT,
	LOAD,
	PRECHARGE,
	START,
	FAULT,
	HALT,
	STOP
};

typedef struct {
    pod_status currentState;
    pod_status nextState;
    uint32_t eventFlags; // bitmask of events
} FSM_t;

FSM_t fsm = { .currentState = INIT, .nextState = INIT, .eventFlags = 0 };
SensorData sendData;

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
  MX_I2C3_Init();
  MX_I2C4_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

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
  lidarTaskHandle = osThreadNew(StartLidarTask, NULL, &lidarTask_attributes);

  /* creation of mpuTask */
  mpuTaskHandle = osThreadNew(StartMPUTask, NULL, &mpuTask_attributes);

  /* creation of thermistorsTask */
  thermistorsTaskHandle = osThreadNew(StartThermistorsTask, NULL, &thermistorsTask_attributes);

  /* creation of LEDTask */
  LEDTaskHandle = osThreadNew(StartLEDTask, NULL, &LEDTask_attributes);

  /* creation of INATask */
  INATaskHandle = osThreadNew(StartINATask, NULL, &INATask_attributes);

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


  SensorData g_sensorData;

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


void init_actions(SensorData *data) {
	// INIT actions
    data->lidar_dist = 0;
    data->roll = 0;
    data->pitch = 0;
    for(int i=0; i<8; i++) data->thermistors[i] = 0;

    // Turn off LEDs or set to INIT color
    WS2812_SetAll(0, 0, 50);
    WS2812_Start();

    printf("INIT actions completed\r\n");
}

void load_actions(SensorData *data) {
	// LOAD actions

    printf("LOAD actions completed\r\n");
}

void precharge_actions(SensorData *data) {
	// PRECHARGE actions

    printf("PRECHARGE actions completed\r\n");
}

void start_actions(SensorData *data) {
	// START actions

    printf("START actions completed\r\n");
}

void stop_actions(SensorData *data) {
	// STOP actions

    printf("STOP actions completed\r\n");
}

void fault_actions(SensorData *data) {
	// FAULT actions

    printf("FAULT actions completed\r\n");
}

void halt_actions(SensorData *data) {
	// HALT actions

    printf("HALT actions completed\r\n");
}


void StartFSMTask(void *argument) {
	unint32_t flags = 0;

	for (;;) {
		// check for manual override first
		if (flags & EVT_USER_INIT)	fsm.currentState = INIT;
		else if (flags & EVT_USER_LOAD)  fsm.currentState = LOAD;
		else if (flags & EVT_USER_PRECHARGE) fsm.currentState = PRECHARGE;
		else if (flags & EVT_USER_START) fsm.currentState = START;
		else if (flags & EVT_USER_STOP)  fsm.currentState = STOP;
		else if (flags & EVT_USER_FAULT) fsm.currentState = FAULT;


		switch(fsm.currentState) {
		case INIT:
			init_actions(&sensorData);
			if(fsm.eventFlags & EVT_LOAD_COMPLETE) {
				printf("INIT -> LOAD\n");
				fsm.nextState = LOAD;
				fsm.eventFlags &= ~EVT_LOAD_COMPLETE;
			}
			break;
		case LOAD:
			load_actions(&sensorData);
			if(fsm.eventFlags & EVT_PRECHARGE_COMPLETE) {
				printf("LOAD -> PRECHARGE\n");
				fsm.nextState = PRECHARGE;
				fsm.eventFlags &= ~EVT_PRECHARGE_COMPLETE;
			}
			break;
		case PRECHARGE:
			precharge_actions(&sensorData);
			if(flags & EVT_START_COMPLETE) {
				printf("PRECHARGE --> START\n");
				fsm.currentState = START;
			}
		case START:
			start_actions(&sensorData);
			if(flags & EVT_STOP_COMPLETE) {
				printf("START --> STOP\n");
				fsm.currentState = START;
			}
			break;
		case STOP:
			stop_actions(&sensorData);
			if(flags & EVT_FAULT_COMPLETE) {
				printf("STOP --> FAULT\n");
				fsm.currentState = START;
			}
			break;
		case FAULT:
			fault_actions(&sensorData);
			fsm.currentState = START;
			break;
		case HALT:
			halt_actions(&sensorData);
			break;
		}
		flags = 0;
		osDelay(50); // non-blocking
	}
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
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.Timing = 0x00602173;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief I2C4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C4_Init(void)
{

  /* USER CODE BEGIN I2C4_Init 0 */

  /* USER CODE END I2C4_Init 0 */

  /* USER CODE BEGIN I2C4_Init 1 */

  /* USER CODE END I2C4_Init 1 */
  hi2c4.Instance = I2C4;
  hi2c4.Init.Timing = 0x10707DBC;
  hi2c4.Init.OwnAddress1 = 0;
  hi2c4.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c4.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c4.Init.OwnAddress2 = 0;
  hi2c4.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c4.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c4.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c4) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c4, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c4, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C4_Init 2 */

  /* USER CODE END I2C4_Init 2 */

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
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
void lidar_config(int configur)
{
    // configuration setting for lidar
    cmd[0] = 0x04;
    HAL_I2C_Mem_Write(&hi2c4,LIDAR_ADD ,0x00,1,cmd,1,0x100);
    switch(configur)
    {
    case 0:
        //default mode , balance mode
        cmd[0]=0x80;
        HAL_I2C_Mem_Write(&hi2c4,LIDAR_ADD ,0x02,1,cmd,1,0x1000);
        cmd[0]=0x04;
        HAL_I2C_Mem_Write(&hi2c4,LIDAR_ADD ,0x04,1,cmd,1,0x1000);
        cmd[0]=0x00;
        HAL_I2C_Mem_Write(&hi2c4,LIDAR_ADD ,0x1c,1,cmd,1,0x1000);
        break;


    case 1:
        //short range, high speed
        cmd[0]=0x1d;
        HAL_I2C_Mem_Write(&hi2c4,LIDAR_ADD ,0x02,1,cmd,1,0x1000);
        cmd[0]=0x08;
        HAL_I2C_Mem_Write(&hi2c4,LIDAR_ADD ,0x04,1,cmd,1,0x1000);
        cmd[0]=0x00;
        HAL_I2C_Mem_Write(&hi2c4,LIDAR_ADD ,0x1c,1,cmd,1,0x1000);
        break;


    case 2:
        //default range, higher speed short range
        cmd[0]=0x80;
        HAL_I2C_Mem_Write(&hi2c4,LIDAR_ADD ,0x02,1,cmd,1,0x1000);
        cmd[0]=0x08;
        HAL_I2C_Mem_Write(&hi2c4,LIDAR_ADD ,0x04,1,cmd,1,0x1000);
        cmd[0]=0x00;
        HAL_I2C_Mem_Write(&hi2c4,LIDAR_ADD ,0x1c,1,cmd,1,0x1000);
        break;




    case 3:
        //maximum Range
        cmd[0]=0xff;
        HAL_I2C_Mem_Write(&hi2c4,LIDAR_ADD ,0x02,1,cmd,1,0x1000);
        cmd[0]=0x08;
        HAL_I2C_Mem_Write(&hi2c4,LIDAR_ADD ,0x04,1,cmd,1,0x1000);
        cmd[0]=0x00;
        HAL_I2C_Mem_Write(&hi2c4,LIDAR_ADD ,0x1c,1,cmd,1,0x1000);
        break;


    case 4:
        //high sensitivity detection, high  measurement
        cmd[0]=0x80;
        HAL_I2C_Mem_Write(&hi2c4,LIDAR_ADD ,0x02,1,cmd,1,0x1000);
        cmd[0]=0x08;
        HAL_I2C_Mem_Write(&hi2c4,LIDAR_ADD ,0x04,1,cmd,1,0x1000);
        cmd[0]=0x80;
        HAL_I2C_Mem_Write(&hi2c4,LIDAR_ADD ,0x1c,1,cmd,1,0x1000);
        break;


    case 5:
        //low sensitivity detection , low  measurement
        cmd[0]=0x80;
        HAL_I2C_Mem_Write(&hi2c4,LIDAR_ADD ,0x02,1,cmd,1,0x1000);
        cmd[0]=0x08;
        HAL_I2C_Mem_Write(&hi2c4,LIDAR_ADD ,0x04,1,cmd,1,0x1000);
        cmd[0]=0xb0;
        HAL_I2C_Mem_Write(&hi2c4,LIDAR_ADD ,0x1c,1,cmd,1,0x1000);
        break;
    }
}


int retrieve_lidar_distance()
{
    cmd[0]=0x04;
    HAL_I2C_Mem_Write(&hi2c4,LIDAR_ADD ,0x00,1,cmd,1,100);
    cmd[0]=0x8f;
    HAL_I2C_Master_Transmit(&hi2c4,LIDAR_ADD,cmd,1,100);
    HAL_I2C_Master_Receive(&hi2c4,LIDAR_ADD,data,2,100);
    m_distance = (data[0]<<8)|(data[1]);
    return m_distance ;
}

void convert_millis_to_hms(uint32_t total_milliseconds, uint32_t* hours, uint32_t* minutes, uint32_t* seconds) {
    uint32_t total_seconds = total_milliseconds / 1000;

    *seconds = total_seconds % 60;
    *minutes = (total_seconds / 60) % 60;
    *hours = total_seconds / 3600;
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
	printf("Lidar Task Starting...\r\n");

	/* Configure lidar once */
	lidar_config(4);
	uint32_t object_distance;

	char uart_tx_buff[100];

	for(;;)
	{
		object_distance = retrieve_lidar_distance();
		sprintf(uart_tx_buff, "LIDAR Distance: %lu cm\r\n", object_distance);
		HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t*)uart_tx_buff, strlen(uart_tx_buff), 100);

		//printf("Lidar task ALIVE");
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
	  printf("MPU Task Starting...\r\n");

	  // MPU initialization
	  MPU6050_Initialization(&hi2c3);
	  printf("Finished MPU initialization.\r\n");
	  float roll = 0, pitch = 0;
	  const float alpha = 0.98f;   // 98% gyro, 2% accelerometer
	  uint32_t lastTick = osKernelGetTickCount();
	  char uart_tx_buff[150];

	  /* Infinite loop */
	  for(;;)
	  {
		  if(MPU6050_DataReady()) {
			  uint32_t now = osKernelGetTickCount();
			  float dt = (now - lastTick) / 1000.0f;
			  lastTick = now;

			  MPU6050_ProcessData(&MPU6050);
			  float acc_roll  = atan2(MPU6050.acc_y, MPU6050.acc_z) * 180.0f / M_PI;
			  float acc_pitch = atan2(-MPU6050.acc_x, sqrtf(MPU6050.acc_y*MPU6050.acc_y +MPU6050.acc_z*MPU6050.acc_z))
								  * 180.0f / M_PI;

			  // 2. Integrate gyro
			  roll  += MPU6050.gyro_x * dt;
			  pitch += MPU6050.gyro_y * dt;

			  // 3. Complementary filter
			  roll  = alpha * roll  + (1 - alpha) * acc_roll;
			  pitch = alpha * pitch + (1 - alpha) * acc_pitch;

			  sprintf(uart_tx_buff, "Roll: %f  Pitch: %f\r\n", roll, pitch);
			  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);
			  sprintf(uart_tx_buff, "Acc | x: %f, y: %f, z: %f\r\n", MPU6050.acc_x, MPU6050.acc_y, MPU6050.acc_z);
			  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);
			  sprintf(uart_tx_buff, "Gyro | x: %f, y: %f, z: %f\r\n", MPU6050.gyro_x, MPU6050.gyro_y, MPU6050.gyro_z);
			  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);
			  sprintf(uart_tx_buff, "Acc Raw | x: %d, y: %d, z: %d\r\n", MPU6050.acc_x_raw, MPU6050.acc_y_raw, MPU6050.acc_z_raw);
			  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);

			 }
	  //printf("MPU task initialized!");
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
	    int thermistor_count = 8;
	    uint16_t rawValues[thermistor_count];
	    char uart_tx_buff[100];  // <-- buffer for sprintf

	    hadc1.Init.ContinuousConvMode = ENABLE; // DMA in circular mode
	    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)rawValues, thermistor_count);

	  /* Infinite loop */
	  for(;;)
	  {
		  if (convCompleted) {
			  convCompleted = 0;
			  time_elapsed = HAL_GetTick() - startTime;
			  convert_millis_to_hms(time_elapsed, &hours, &minutes, &seconds);
			  for (int i=0; i<thermistor_count; i++) {
				  float value = ntc_convertToC(rawValues[i]);
				  sprintf(uart_tx_buff, "Thermistor %d,%.2f,%02lu:%02lu:%02lu\r\n", i, value, hours, minutes, seconds);
				  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);
			  }
		  }
		  //printf("Thermistors ALIVE");
		  osDelay(300);
	  }
	  /* USER CODE END StartThermistorsTask */
}

/* USER CODE BEGIN Header_StartLEDTask */
/**
* @brief Function implementing the LEDTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLEDTask */
void StartLEDTask(void *argument)
{
  /* USER CODE BEGIN StartLEDTask */
	WS2812_Init(&htim1, TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
	/* Infinite loop */
	for(;;)
	  {
	  printf("Pink\r\n");
	  WS2812_SetAll(128, 0, 64);
	  WS2812_Start();
	  osDelay(500);
	  printf("Blue\r\n");
	  WS2812_SetAll(0, 0, 180);
	  WS2812_Start();
	  osDelay(500);
	  printf("Yellow\r\n");
	  WS2812_SetAll(128, 128, 0);
	  WS2812_Start();
	  osDelay(500);
	  printf("Green\r\n");
	  WS2812_SetAll(0, 180, 0);
	  WS2812_Start();
	  osDelay(500);
	  printf("Red\r\n");
	  WS2812_SetAll(180, 0, 0);
	  WS2812_Start();
	  osDelay(500);

		//printf("LED Task Alive!");
		osDelay(300);
	  }
  /* USER CODE END StartLEDTask */
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
	  INA219_t ina219;
	  uint16_t vbus, vshunt, current, power;

	  INA219_t ina219_1;
	  uint16_t vbus1, vshunt1, current1, power1;
	  char uart_tx_buff[100];
	  if (!INA219_Init(&ina219, &hi2c1, INA219_ADDRESS)) {
		  Error_Handler();
	  }

	  if (!INA219_Init(&ina219_1, &hi2c1, INA219_ADDRESS1)) {
		  Error_Handler();
	  }

	  /* Infinite loop */
	  for(;;)
	  {
		  vbus = INA219_ReadBusVoltage(&ina219);
		  vshunt = INA219_ReadShuntVoltage(&ina219);
		  current = INA219_ReadCurrent(&ina219);
		  power = INA219_ReadPower(&ina219);

		  sprintf(uart_tx_buff, "vbus: %hu mV\r\n",vbus);
		  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);

		  sprintf(uart_tx_buff, "vShunt: %hu mV\r\n",vshunt);
		  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);

		  sprintf(uart_tx_buff, "current: %hu mA\r\n",current);
		  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);

		  sprintf(uart_tx_buff, "power: %hu mW\r\n",power );
		  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);

		  vbus1 = INA219_ReadBusVoltage(&ina219_1);
		  vshunt1 = INA219_ReadShuntVoltage(&ina219_1);
		  current1 = INA219_ReadCurrent(&ina219_1);
		  power1 = INA219_ReadPower(&ina219_1);

		  sprintf(uart_tx_buff, "vbus 1: %hu mV\r\n",vbus1);
		  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);

		  sprintf(uart_tx_buff, "vShunt 1: %hu mV\r\n",vshunt1);
		  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);

		  sprintf(uart_tx_buff, "current 1: %hu mA\r\n",current1);
		  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);

		  sprintf(uart_tx_buff, "power 1: %hu mW\r\n\n",power1);
		  HAL_UART_Transmit(&hcom_uart[COM1], (uint8_t *)uart_tx_buff, strlen(uart_tx_buff), 100);

		//printf("INA Task Alive!");
		osDelay(300);
	  }
  /* USER CODE END StartINATask */
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
