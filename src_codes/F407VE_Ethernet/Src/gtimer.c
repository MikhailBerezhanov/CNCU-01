/**
  ******************************************************************************
  * @file           : gtimer.c				  
  * @brief          : Module working with general purpose timer - generates
											interrupts every [resolution] ms\us and decrements
											user timers in ISR. 
  ******************************************************************************
*/
#include "stm32f4xx_hal.h"
#include "gtimer.h"

TIM_HandleTypeDef xTIM10;

/* Exported variables --------------------------------------------------------*/
extern volatile uint32_t eep_timer;
extern volatile uint32_t dhcp_timer;

/* Private functions (HAL_DeInit callback)------------------------------------*/
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim_base)
{
  if(htim_base->Instance==TIM10)
  {
    /* Peripheral clock disable */
    __HAL_RCC_TIM10_CLK_DISABLE();

    /* TIM10 interrupt DeInit */
    HAL_NVIC_DisableIRQ(TIM1_UP_TIM10_IRQn);
  }
	else if(htim_base->Instance==TIM11)
	{
    /* Peripheral clock disable */
    __HAL_RCC_TIM11_CLK_DISABLE();

    /* TIM10 interrupt DeInit */
    HAL_NVIC_DisableIRQ(TIM1_TRG_COM_TIM11_IRQn);
  }	
}


/**
	@brief Add user timers handling here
 */
void Gtimer_ISR(void)
{
	if (eep_timer) eep_timer--;
	if (dhcp_timer) dhcp_timer--; 
}

static uint16_t CalculatePrescaler (ERESOL resolution)
{
	uint32_t tick_freq = 0;
	uint16_t presc = 0;
	
	switch (resolution)
	{
		case us_resolution:
			tick_freq = 2000000;	// we want tick = 0.5 us
			break;
		
		case ms_resolution:
		default:
			tick_freq = 10000;		// we want tick = 0.1 ms			
			break;
	}
	
	// APB2 timer clocks = (PCLK2 x 2) or (PCLK2 x 1) depending on APB2 prescaler
	if (APBPrescTable[(RCC->CFGR & RCC_CFGR_PPRE2)>>RCC_CFGR_PPRE2_Pos])
		presc = (CNCU.Config.PCLK2 * 2) / tick_freq - 1;
	else
		presc = CNCU.Config.PCLK2 / tick_freq - 1;
	
	return presc;
}

/**
	@brief Initialization of general timer for tim_tick = 1 ms or 1 us
 */ 
uint8_t Gtimer_Init (ERESOL resolution)
{
  __HAL_RCC_TIM10_CLK_ENABLE();							// CLK from APB2
	
  xTIM10.Instance = TIM10;
	xTIM10.Init.Prescaler = CalculatePrescaler(resolution);
  if (resolution == ms_resolution)
	{							
		xTIM10.Init.Period = 9;
	}
  else	
	{		
		xTIM10.Init.Period = 1;
	}
  xTIM10.Init.CounterMode = TIM_COUNTERMODE_UP;
  xTIM10.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  if (HAL_TIM_Base_Init(&xTIM10) != HAL_OK)
  {
    return 1;
  }
  
  /* TIM1_UP_TIM10_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(TIM1_UP_TIM10_IRQn, GTIM_PreP, GTIM_SubP);
  HAL_NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
  
  return 0;
}

/**
	@brief Start generating IRQ when timer's Period is reached
 */ 
void Gtimer_Start (void)
{
  HAL_TIM_Base_Start_IT(&xTIM10);
}

/**
	@brief Stop generating IRQ
 */ 
void Gtimer_Stop (void)
{
  HAL_TIM_Base_Stop_IT(&xTIM10);
}

/**
	@brief Get Gtimer's current counter
 */
inline uint16_t Gtimer_GetCnt (void)
{
	return(__HAL_TIM_GET_COUNTER(&xTIM10));	
}
