/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : balie.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "balie.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdbool.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DEBOUNCE_MS 150

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c3;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim15;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/** @brief Vlag wat aangeeft of de knop ingedrukt was */
volatile bool knopknop_flag = false;
char baliedata[2];
/** @brief enum voor het bewaren van de huidige state. Begint op rood */
enum { STATE_RED = 0, STATE_GREEN, STATE_BLUE, STATE_SEND } state = STATE_RED;
/** @brief Opslag voor de rgb kleuren 0-255 */
uint8_t rgb[3] = { 0, 0, 0 };
static int16_t lastCnt = 32768;
/** @brief Pointer voor het gemak bij het printen */
static const char *channel_names[] = { "Red", "Green", "Blue" };
static uint32_t lastKeyTick = 0;

int globv = 0; // I2C CO2 Oscar

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM15_Init(void);
static void MX_I2C3_Init(void);
/* USER CODE BEGIN PFP */

// I2C CO2 Oscar
void Init_iaq()
{
    uint8_t Command[2] = {0x20,0x03};//Init commando van de SGP30 om elke seconde Co2/TVOC te gaan meten
    HAL_I2C_Master_Transmit(&hi2c3,0x58<<1,Command,2, 100);
}
// I2C CO2 Oscar
/**
 * @brief Meet de indoor air quality (IAQ) met de SGP30 sensor.
 *
 * Stuur een read commando aan de SGP30 sensor  I2C en lees de CO2 en TVOC waardes.
 * De data wordt over UART geprint en de CO2 niveau gereturned.
 *
 * @return uint16_t De gemeten CO2 niveau (in ppm).
 */
uint16_t Measure_iaq()
{
    uint8_t Data[6];//Data = {Co2 MSB, Co2 LSB, Checksum, TVOC MSB, TVOC LSB, Checksum}: de structuur van de ontvange data

    uint8_t Command[2] = {0x20,0x08};//Lees de iaq waarde af commando

    //ic2 connectie, adress SGP30, commando, groote van t commando in bytes, timeout
    HAL_I2C_Master_Transmit(&hi2c3,0x58<<1,Command,2, 100);//stuur 't commando naar de SGP30
    HAL_Delay(20);
    HAL_I2C_Master_Receive(&hi2c3,0x58<<1,Data,6, 100);//stuur een ic2 read msg en lees 6 bytes data af, sla die data op in Data[6]

    char msg[100];
    int len=snprintf(msg,100,"IAQ Data Co2:%lu TVOC:%lu\n\r",Data[0]*256+Data[1],Data[3]*256+Data[4]);
    HAL_UART_Transmit(&huart2, msg, len, HAL_MAX_DELAY);
    return Data[0]*256+Data[1];//data[0] is de eerste byte die gestuurd wordt en bevat de msb, data[1] de lsb
    //*256 is dus om t naar 1 getal te brengen
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_TIM1_Init();
  MX_TIM15_Init();
  MX_I2C3_Init();
  /* USER CODE BEGIN 2 */
   Init_iaq(); // I2C CO2 Oscar
  // Kleuren van de knoppen:
  // Oranje = knopknop(deur)
  // Blauw = schemerknop
  // Oranje = stoelknop

  char schemervar[] = "\r[STATE] SCHEMERLAMP\r";
  int oudetoestand = 0;

  char stoelvar[] = "\r[STATE] STOEL\r";
  int oudetoestandstoel = 0;

  char test[] = "THIS IS A TEST\r";
  char brandvar[] = "\r[STATE] BRAND?\r";


  HAL_UART_Receive_IT(&huart1, baliedata, 1);
  HAL_TIM_Encoder_Start_IT(&htim1, TIM_CHANNEL_ALL);
  __HAL_TIM_SET_COUNTER(&htim1, lastCnt);

   HAL_TIM_Base_Start_IT(&htim15); //I2C CO2 Oscar




  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
   /**
    * @brief Main application loop.
    *
    * Oneindige loop:
    * - Handelt deur-open via `knopknop_flag`, verstuurt commandos over UART.
    * - Checkt de state van de schemerknop "schemerknop" en verstuurt een commando als deze ingedrukt is.
    * - Checkt de state van de "stoelknop" en verstuurt een commando als deze ingedrukt is.
    * - Periodieke check van de indoor air quality (IAQ) als `globv == 1`. Als CO2 >= 550 ppm,
    *   verstuur een BRAND bericht over UART.
    */
  while (1)
  {
	  if (knopknop_flag) {
		  knopknop_flag = false;
		  const char msg[] = "\r[OPEN]\r!BEZOEKER\r";
		  HAL_UART_Transmit(&huart1, (uint8_t*)msg, sizeof(msg), HAL_MAX_DELAY);
		  HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, HAL_MAX_DELAY);
		  HAL_UART_Transmit(&huart2, "\n", sizeof(char), HAL_MAX_DELAY);
	    }

	  // Schemerlamp commando sturen
	  if(!(HAL_GPIO_ReadPin(GPIOA, schemerknop_Pin)) && (oudetoestand)){
		  	HAL_Delay(100);
            HAL_UART_Transmit(&huart1, (uint8_t*)&schemervar, sizeof(schemervar), HAL_MAX_DELAY);
            HAL_UART_Transmit(&huart2, (uint8_t*)&schemervar, sizeof(schemervar), HAL_MAX_DELAY);
            HAL_UART_Transmit(&huart2, "\n", sizeof(char), HAL_MAX_DELAY);

            oudetoestand = 0;
	  } else if(HAL_GPIO_ReadPin(GPIOA, schemerknop_Pin)){
		  oudetoestand = 1;
	  }

	  // Stoel commando sturen
	  if(!(HAL_GPIO_ReadPin(GPIOA, stoelknop_Pin)) && (oudetoestandstoel)){
		  HAL_Delay(100);
	      HAL_UART_Transmit(&huart1, (uint8_t*)&stoelvar, sizeof(stoelvar), HAL_MAX_DELAY);
	      HAL_UART_Transmit(&huart2, (uint8_t*)&stoelvar, sizeof(stoelvar), HAL_MAX_DELAY);
	      HAL_UART_Transmit(&huart2, "\n", sizeof(char), HAL_MAX_DELAY);

	      oudetoestandstoel = 0;
	  } else if(HAL_GPIO_ReadPin(GPIOA, stoelknop_Pin)){
		  oudetoestandstoel = 1;
	  }


	  // I2C CO2 Oscar
	  if(globv == 1) {
		  if (Measure_iaq() >= 550) {
			  //stuur brand over bus
		      HAL_UART_Transmit(&huart1, (uint8_t*)&brandvar, sizeof(brandvar), HAL_MAX_DELAY);
		      HAL_UART_Transmit(&huart2, (uint8_t*)&brandvar, sizeof(brandvar), HAL_MAX_DELAY);

			  }
		  globv = 0;
	  }


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

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 16;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
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
  hi2c3.Init.Timing = 0x00B07CB4;
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
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI1;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 0;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 0;
  if (HAL_TIM_Encoder_Init(&htim1, &sConfig) != HAL_OK)
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
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 32000;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 10000;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM15 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM15_Init(void)
{

  /* USER CODE BEGIN TIM15_Init 0 */

  /* USER CODE END TIM15_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM15_Init 1 */

  /* USER CODE END TIM15_Init 1 */
  htim15.Instance = TIM15;
  htim15.Init.Prescaler = 32000;
  htim15.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim15.Init.Period = 1000;
  htim15.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim15.Init.RepetitionCounter = 0;
  htim15.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim15) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim15, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim15, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM15_Init 2 */

  /* USER CODE END TIM15_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(lamp_GPIO_Port, lamp_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : schemerknop_Pin stoelknop_Pin */
  GPIO_InitStruct.Pin = schemerknop_Pin|stoelknop_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : knopknop_Pin */
  GPIO_InitStruct.Pin = knopknop_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(knopknop_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : key_Pin */
  GPIO_InitStruct.Pin = key_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(key_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PB3 lamp_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_3|lamp_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
 * @brief Callback functie getriggered door input capture event op TIM1.
 *
 * Pas de RGB waardes van de huide channel op bassis van de encoder positie.
 * Verstuurt de nieuwe waarde over UART als er verandering is.
 *
 * @param htim Pointer tot de TIM handle die de callback triggered.
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
	 if (htim->Instance == TIM1) {
		 // read and reset teller
		 int16_t cnt  = (int16_t)__HAL_TIM_GET_COUNTER(htim);
		 int16_t diff = (cnt - lastCnt);
		 // recentre for next delta
		  __HAL_TIM_SET_COUNTER(&htim1, lastCnt);

		 // adjust RGB
		  if (diff != 0) {
		       uint8_t old = rgb[state];
		       int32_t tmp = rgb[state] + 10 * (diff > 0 ? 1 : -1);
		       if (tmp < 0)      tmp = 0;
		       else if (tmp > 255) tmp = 255;
		       rgb[state] = (uint8_t)tmp;

		       // print only when value was changed
		       if (rgb[state] != old) {
		         char buf[32];
		         int len = snprintf(buf, sizeof(buf),
		                            "%s = %u       \r",
		                            channel_names[state],
		                            rgb[state]);
		         HAL_UART_Transmit(&huart2, (uint8_t*)buf, len, HAL_MAX_DELAY);
		       }
		  }
	 }

}
/**
 * @brief Callback functie getriggered door de timer period elapsed interrupt.
 *
 * - If getriggered door TIM2: stopt de timer en verstuur een byte over UART.
 * - If getriggered door TIM15: sets een globale vlag om de IAQ meting te triggeren.
 *
 * @param htim Pointer naar de TIM handle die de callback triggered.
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim){
	if(htim == &htim2){
		char var = 3;
		HAL_GPIO_WritePin(GPIOB, lamp_Pin, RESET);
		HAL_TIM_Base_Stop_IT(&htim2);
        HAL_UART_Transmit(&huart1, (uint8_t*)&var, sizeof(var), HAL_MAX_DELAY);

	}

	// I2C CO2 Oscar
	if(htim == &htim15){
		globv = 1;
	}

}
/**
 * @brief Callback functie getriggered wanneer een UART receive interrupt voltooid is.
 *
 * Handelt binnenkomende UART data van huart1. Als '@' ontvangen is, gaat de lamp aan
 * en start TIM2. Her-enabled de UART receive interrupt.
 *
 * @param huart Pointer naar de UART handle die de callback getriggered heeft.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart){
	if(huart == &huart1){
		if(baliedata[0] == '@'){
		HAL_GPIO_WritePin(GPIOB, lamp_Pin, SET);
		HAL_TIM_Base_Start_IT(&htim2);
		__HAL_TIM_SET_COUNTER(&htim2, 0);
		}
	}
	HAL_UART_Receive_IT(&huart1, baliedata, 1);

}


/**
 * @brief Callback function getriggered bij een external GPIO interrupt.
 *
 * - Voor knopknop_Pin: Doet de lamp uit en stopt TIM2 als de lamp aan is.
 * - Voor key_Pin: Debounced input en loopt door de RGB states. Op de laatste state,
 *   verstuurt het de RGB data over de UART en word the state reset naar de begin state.
 *
 * @param GPIO_Pin Pin dat de interrupt getriggered heeft.
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {

    if ((GPIO_Pin == knopknop_Pin) && (HAL_GPIO_ReadPin(GPIOB, lamp_Pin) == 1) ) {
    	if (HAL_GPIO_ReadPin(GPIOB, lamp_Pin) == GPIO_PIN_SET) {
    		knopknop_flag = true;
    		HAL_GPIO_WritePin(GPIOB, lamp_Pin, RESET);
    		HAL_TIM_Base_Stop_IT(&htim2);
    	}
    }

    else  if (GPIO_Pin == key_Pin) {
        uint32_t now = HAL_GetTick();
        char newline[] = "\n";

        // if elapsedtime < DEBOUNCE_MS since last press, ignore
        if (now - lastKeyTick < DEBOUNCE_MS)
            return;
        lastKeyTick = now;
        HAL_UART_Transmit(&huart2, (uint8_t*) newline, sizeof(newline), HAL_MAX_DELAY);


        state++;
        if (state == STATE_SEND) {
        	char MSG[] = "[RGB] XXX\r";
        	MSG[6] = rgb[0];
        	MSG[7] = rgb[1];
        	MSG[8] = rgb[2];
            HAL_UART_Transmit(&huart1, MSG, sizeof(MSG), HAL_MAX_DELAY);

            char buf[32];
            int len = snprintf(buf, sizeof(buf),
                               "Sent RGB = %u, %u, %u\r\n",
                               rgb[0], rgb[1], rgb[2]);

            HAL_UART_Transmit(&huart2, (uint8_t*)buf, len, HAL_MAX_DELAY);

            // wrap back
            state = STATE_RED;
        }
    }

}

/* USER CODE END 4 */

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

#ifdef  USE_FULL_ASSERT
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
