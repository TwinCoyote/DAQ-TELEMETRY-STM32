/**
 ******************************************************************************
 * @file    main.c
 * @brief   DAQ telemetry demo - Wokwi port (STM32C031C6 / Nucleo-C031C6)
 *
 * Same application logic as the STM32F401RE firmware in ../Core:
 *  - A timer fires at 10 Hz and increments `sample_count`.
 *  - The main loop reads ADC channel 0 (PA0) for every pending tick and
 *    sends "ADC: <raw> | Voltage: <V> V" over USART2 (PA2/PA3, 115200 8N1).
 *
 * Differences from the F401 version (Wokwi has no STM32F4 support):
 *  - TIM2 -> TIM3 (the C031 has no TIM2), 48 MHz HSI clock.
 *  - ADC/GPIO/UART are initialised with the STM32C0 HAL API.
 ******************************************************************************
 */
#include "main.h"
#include <stdio.h>
#include <string.h>

#define ADC_MAX   4095UL
#define VREF_CV   330UL /* 3.30 V expressed in centivolts */

ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim3;
UART_HandleTypeDef huart2;

volatile uint32_t sample_count = 0U;

void SystemClock_Config(void);
static void MX_ADC1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM3_Init(void);
HAL_StatusTypeDef ADC_Read(uint32_t *value);

int main(void)
{
	uint32_t processed_count = 0U;
	uint32_t rawValue = 0U;
	char message[48];
	uint32_t centivolts = 0U;
	static const char banner[] = "DAQ Telemetry (Wokwi / STM32C031C6)\r\n";

	HAL_Init();
	SystemClock_Config();

	MX_ADC1_Init();
	MX_USART2_UART_Init();
	MX_TIM3_Init();

	HAL_UART_Transmit(&huart2, (uint8_t*) banner, strlen(banner), 100);

	HAL_TIM_Base_Start_IT(&htim3);

	while (1) {
		if (sample_count > processed_count) {
			if (ADC_Read(&rawValue) == HAL_OK) {
				/* Integer math (rounded) instead of float printf: the C031C6
				 * only has 32 KB of flash and "%f" would not leave room. */
				centivolts = (rawValue * VREF_CV + ADC_MAX / 2U) / ADC_MAX;
				int len = snprintf(message, sizeof(message),
						"ADC: %lu | Voltage: %lu.%02lu V\r\n",
						(unsigned long) rawValue,
						(unsigned long) (centivolts / 100U),
						(unsigned long) (centivolts % 100U));
				HAL_UART_Transmit(&huart2, (uint8_t*) message, len, 100);
			}
			processed_count++;
		}
	}
}

void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
	RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
		Error_Handler();
	}
}

static void MX_ADC1_Init(void)
{
	ADC_ChannelConfTypeDef sConfig = { 0 };

	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
	hadc1.Init.Resolution = ADC_RESOLUTION_12B;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.ScanConvMode = ADC_SCAN_SEQ_FIXED;
	hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	hadc1.Init.LowPowerAutoWait = DISABLE;
	hadc1.Init.LowPowerAutoPowerOff = DISABLE;
	hadc1.Init.ContinuousConvMode = DISABLE;
	hadc1.Init.NbrOfConversion = 1;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc1.Init.DMAContinuousRequests = DISABLE;
	hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
	hadc1.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_79CYCLES_5;
	hadc1.Init.SamplingTimeCommon2 = ADC_SAMPLETIME_1CYCLE_5;
	hadc1.Init.OversamplingMode = DISABLE;
	hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
	if (HAL_ADC_Init(&hadc1) != HAL_OK) {
		Error_Handler();
	}

	sConfig.Channel = ADC_CHANNEL_0;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
		Error_Handler();
	}
}

/* 48 MHz / (4799 + 1) / (999 + 1) = 10 Hz, same rate as the F401 version */
static void MX_TIM3_Init(void)
{
	TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };

	htim3.Instance = TIM3;
	htim3.Init.Prescaler = 4799;
	htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim3.Init.Period = 999;
	htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim3) != HAL_OK) {
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK) {
		Error_Handler();
	}
}

static void MX_USART2_UART_Init(void)
{
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 115200;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
	huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huart2) != HAL_OK) {
		Error_Handler();
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == TIM3) {
		sample_count++;
	}
}

HAL_StatusTypeDef ADC_Read(uint32_t *value)
{
	HAL_StatusTypeDef status_start, status_conversion;

	status_start = HAL_ADC_Start(&hadc1);

	if (status_start == HAL_OK) {
		status_conversion = HAL_ADC_PollForConversion(&hadc1, 100);
		if (status_conversion == HAL_OK) {
			*value = HAL_ADC_GetValue(&hadc1);
		}
		HAL_ADC_Stop(&hadc1);
		return status_conversion;
	}
	return status_start;
}

void Error_Handler(void)
{
	__disable_irq();
	while (1) {
	}
}
