/**
  ******************************************************************************
  * @file           : leds.c
  * @brief          : Module for controlling CNCU leds
  ******************************************************************************
*/
#include <string.h>
#include "leds.h"

TIM_HandleTypeDef xTIM11;
static volatile ELEDS led_cnt = GEN_LED;		// cnt for IRQ_RunningBlink mode
static _Bool RunningBlink_EN = 0;
static volatile uint32_t RunningBlink_Period = RUNNING_BLINK_PERIOD;	 
static Led_t Leds[NUMBER_OF_LEDS];

static uint16_t CalculatePrescaler (void)
{
	uint32_t tick_freq = 10000; // we want tick = 0.1 ms
	
	// APB2 timer clocks = (PCLK2 x 2) or (PCLK2 x 1) depending on APB2 prescaler
	if (APBPrescTable[(RCC->CFGR & RCC_CFGR_PPRE2)>>RCC_CFGR_PPRE2_Pos])
		return ((HAL_RCC_GetPCLK2Freq() * 2) / tick_freq - 1);
	else
		return (HAL_RCC_GetPCLK2Freq() / tick_freq - 1);
}

/**
	@brief GPIO initialization for using leds
 */
uint8_t Leds_Init (void)
{
  GPIO_InitTypeDef GPIO_InitStruct;
	
  __HAL_RCC_GPIOE_CLK_ENABLE();
	
  /*Configure GPIO pin Output Level (LEDS OFF by default)*/
  HAL_GPIO_WritePin(GPIOE, ERR_LED_Pin | CAN_LED_Pin | GEN_LED_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : PE3 PE4 PE5 */
  GPIO_InitStruct.Pin = ERR_LED_Pin | CAN_LED_Pin | GEN_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	
  // Init Timer 11 for blinking purposes
  __HAL_RCC_TIM11_CLK_ENABLE();						// APB2 CLK Bus

	// Timer tick every 1 ms
  xTIM11.Instance = TIM11;
  xTIM11.Init.Prescaler = CalculatePrescaler();
  xTIM11.Init.CounterMode = TIM_COUNTERMODE_UP;
  xTIM11.Init.Period = 9;
  xTIM11.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  if (HAL_TIM_Base_Init(&xTIM11) != HAL_OK)
  {
    return INIT_ERR;
  }
	// Init Leds array 
	memset(Leds, 0, sizeof(Led_t)*NUMBER_OF_LEDS);
	Leds[0].type = ERR_LED;
	Leds[1].type = CAN_LED;
	Leds[2].type = GEN_LED;
	
  /* TIM11 interrupt Init */
  HAL_NVIC_SetPriority(TIM1_TRG_COM_TIM11_IRQn, LTIM_PreP, LTIM_SubP);
  HAL_NVIC_EnableIRQ(TIM1_TRG_COM_TIM11_IRQn); 
  HAL_TIM_Base_Start_IT(&xTIM11);
  return CNCU_OK;
}

/**
	@brief Set LED active
 */
void Leds_ON (ELEDS ltype)
{
	switch (ltype)
	{
		case ERR_LED: HAL_GPIO_WritePin(GPIOE, ERR_LED_Pin, GPIO_PIN_RESET);
			break;
		
		case CAN_LED: HAL_GPIO_WritePin(GPIOE, CAN_LED_Pin, GPIO_PIN_RESET);
			break;
		
		case GEN_LED: HAL_GPIO_WritePin(GPIOE, GEN_LED_Pin, GPIO_PIN_RESET);
			break;
		
		case ALL:
			HAL_GPIO_WritePin(GPIOE, ERR_LED_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOE, CAN_LED_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOE, GEN_LED_Pin, GPIO_PIN_RESET);
			break;
		
		default: break;
	}
}

/**
	@brief Set LED nonactive
 */
void Leds_OFF (ELEDS ltype)
{
	switch (ltype)
	{
		case ERR_LED: HAL_GPIO_WritePin(GPIOE, ERR_LED_Pin, GPIO_PIN_SET);
			break;
		
		case CAN_LED: HAL_GPIO_WritePin(GPIOE, CAN_LED_Pin, GPIO_PIN_SET);
			break;
		
		case GEN_LED: HAL_GPIO_WritePin(GPIOE, GEN_LED_Pin, GPIO_PIN_SET);
			break;
		
		case ALL:
			HAL_GPIO_WritePin(GPIOE, ERR_LED_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOE, CAN_LED_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOE, GEN_LED_Pin, GPIO_PIN_SET);
			break;
		
		default: break;
	}
}

/**
	@brief Toggle LED
 */
void Leds_Toggle (ELEDS ltype)
{
	switch (ltype)
	{
		case ERR_LED: HAL_GPIO_TogglePin(GPIOE, ERR_LED_Pin);
			break;
		
		case CAN_LED: HAL_GPIO_TogglePin(GPIOE, CAN_LED_Pin);
			break;
		
		case GEN_LED: HAL_GPIO_TogglePin(GPIOE, GEN_LED_Pin);
			break;
		
		case ALL:
			HAL_GPIO_TogglePin(GPIOE, ERR_LED_Pin);
			HAL_GPIO_TogglePin(GPIOE, CAN_LED_Pin);
			HAL_GPIO_TogglePin(GPIOE, GEN_LED_Pin);
			break;
		
		default: break;
	}
}

/**
	@brief force LED to blink with determined period [ms]
 */
void Leds_StartBlink (ELEDS ltype, uint32_t period)
{
	for (int i=0; i < NUMBER_OF_LEDS; i++)
	{
		if (Leds[i].type == ltype)
		{
			Leds[i].blink_en = 1;
			Leds[i].blink_period = period;
			Leds[i].blink_cnt = Leds[i].blink_period; 
		}
	}
}

/**
	@brief force LED to stop blinking
 */
void Leds_StopBlink (ELEDS ltype)
{
	for (int i=0; i < NUMBER_OF_LEDS; i++)
	{
		if (Leds[i].type == ltype)
		{
			Leds[i].blink_en = 0;
			Leds[i].blink_period = 0;
		}
	}
}

/**
	@brief Start TIM11 to generate IRQ -> blink leds one by one
 */
void Leds_StartRunningBlink (void)
{
	Leds_OFF(ALL);
	Leds_StopBlink(GEN_LED);
	Leds_StopBlink(ERR_LED);
	Leds_StopBlink(CAN_LED);
	led_cnt = GEN_LED;
	RunningBlink_Period = RUNNING_BLINK_PERIOD;
	RunningBlink_EN = 1;
}

/**
	@brief Disable generating interrupts
 */
void Leds_StopRunningBlink (void)
{
	RunningBlink_EN = 0;
	Leds_OFF(ALL);
}

/**
	@brief Running Blink Mode
 */
static void Leds_RunningBlink (void)
{
	if (RunningBlink_Period) RunningBlink_Period--;
	else
	{
		RunningBlink_Period = RUNNING_BLINK_PERIOD;
		Leds_Toggle(led_cnt);
		led_cnt--;
		if (led_cnt < ERR_LED) led_cnt = GEN_LED;	
	}
}

void Leds_ISR (void)
{
	// Running mode blinking
	if (RunningBlink_EN) Leds_RunningBlink();
	
	// Self blinking mode
	for (int i=0; i < NUMBER_OF_LEDS; i++)
	{
		if (Leds[i].blink_en)
		{
			if (Leds[i].blink_cnt) Leds[i].blink_cnt--;
			else 
			{
				Leds[i].blink_cnt = Leds[i].blink_period;
				Leds_Toggle(Leds[i].type);
			}
		}
	}
}

