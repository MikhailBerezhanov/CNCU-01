/**
  ******************************************************************************
  * @file           : buzz.c
  * @brief          : Module working with CNCU Buzzer									
  ******************************************************************************
*/
#include "buzz.h"

#define DEBUG_MSG DEBUG_BUZZ
#include "dbgmsg.h"

#define SET_TIM_PERIOD(x)	  	TIM9->ARR = (uint32_t)(x) - 1; \
															TIM9->CCR2 = (uint32_t)((x - 1)/2)		// 50% PWM
#define	F_TIM		1e6					// We want Ftick = 1[Mhz] -> Ttick = 1[us]

static TIM_HandleTypeDef xTIM9;
static volatile sound_buf_t melody;			// User's array of sounds
static volatile uint16_t cnt = 0;				// counter for ISR to scan active sounds
static volatile _Bool enable_buzz = 0;	// Enable ISR routine (Start sound)	 
// Note's duration
static uint16_t ndFULL = 0;							// 1 
static uint16_t ndHALF = 0;							// 1\2
static uint16_t ndQUART = 0;						// 1\4
static uint16_t ndEIGHT = 0;						// 1\8
static uint16_t ndSIXT = 0;							// 1\16
static uint16_t ndTWSEC = 0;						// 1\32
		
/* Private functions (HAL_Init callback)--------------------------------------*/		
/**
* @brief TIM_PWM MSP Initialization
* This function configures the hardware resources used in this example
* @param htim_pwm: TIM_PWM handle pointer
* @retval None
*/
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* htim_pwm)
{
  if(htim_pwm->Instance==TIM9)
  {
    /* Peripheral clock enable */
    __HAL_RCC_TIM9_CLK_ENABLE();
    /* TIM9 interrupt Init */
    //HAL_NVIC_SetPriority(TIM1_BRK_TIM9_IRQn, 5, 0);
    //HAL_NVIC_EnableIRQ(TIM1_BRK_TIM9_IRQn);
  }
}

/**
* @brief TIM_PWM MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param htim_pwm: TIM_PWM handle pointer
* @retval None
*/
void HAL_TIM_PWM_MspDeInit(TIM_HandleTypeDef* htim_pwm)
{
  if(htim_pwm->Instance==TIM9)
  {
    /* Peripheral clock disable */
    __HAL_RCC_TIM9_CLK_DISABLE();

    /* TIM9 interrupt DeInit */
    HAL_NVIC_DisableIRQ(TIM1_BRK_TIM9_IRQn);
  }
}

// ISR routine (Duration calculation and PWM generation)
void BuzzTIM_ISR(void)
{
	if (enable_buzz)
	{
		if(melody.sound[cnt].act)
		{
			melody.sound[cnt].act = 0;
			if(melody.sound[cnt].note)	// Not a Pause
			{
				SET_TIM_PERIOD(melody.sound[cnt].note);
				HAL_TIM_PWM_Start(&xTIM9, TIM_CHANNEL_2); 
			}
		}	
		// Duration calculation
		if(!(melody.sound[cnt].dur--))
		{
			HAL_TIM_PWM_Stop(&xTIM9, TIM_CHANNEL_2);
			cnt++;	// go to next sound
		}
		// Stop playing sounds	
		if(melody.sound_idx == cnt)
		{
			melody.sound_idx = 0;
			cnt = 0;
			enable_buzz = 0;	// self shutdown
		}
	}
}

// Reset sound buffer
static void ResetSoundBuf(void)
{
	memset((void*)&melody, 0, sizeof(sound_buf_t));
	cnt = 0;
}

static uint16_t CalculatePrescaler (uint32_t freq)
{
	// APB2 (PCLK2 x 2) - TIM9 clock source
	return (2 * HAL_RCC_GetPCLK2Freq() / freq);
}

// Init GPIO and Timer
cncu_err_t Buzz_Init(void)
{ 
  TIM_OC_InitTypeDef sConfigOC = {0};
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	
  xTIM9.Instance = TIM9;
  xTIM9.Init.CounterMode = TIM_COUNTERMODE_UP;
  xTIM9.Init.Period = C1 - 1;	
	
  xTIM9.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	xTIM9.Init.Prescaler = CalculatePrescaler(F_TIM);		// 1 tick = 1 [us]
	dbgmsg("-Buzz Timer's Prescaler: %d\r\n", xTIM9.Init.Prescaler);
  if (HAL_TIM_PWM_Init(&xTIM9) != HAL_OK)
	{	
		errmsg("HAL_TIM_PWM_Init() Failed\r\n");
		return INIT_ERR;
	}

  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = xTIM9.Init.Period / 2;	// 50% PWM
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&xTIM9, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
		errmsg("HAL_TIM_PWM_ConfigChannel() Failed\r\n");
    return INIT_ERR;
  }

  __HAL_RCC_GPIOA_CLK_ENABLE();
	/**TIM9 GPIO Configuration    
	PA3     ------> TIM9_CH2 
	*/
	GPIO_InitStruct.Pin = GPIO_PIN_3;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF3_TIM9;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	ResetSoundBuf();
	Buzz_SetTemp(DEFAULT_TEMPO);
	 	
	return CNCU_OK;
}

// Set music speed [bpm] - number of ndQUART in 60 sec
void Buzz_SetTemp(uint16_t tempo)
{				 
	ndFULL = 4*60*1000 / tempo;		// 60 sec * 4 ndQUART 
	ndHALF = ndFULL / 2;
	ndQUART = ndFULL / 4;
	ndEIGHT = ndFULL / 8;
	ndSIXT = ndFULL / 16;
	ndTWSEC = ndFULL / 32;	
}

// Non blocking playing ( Using timer 1tick = 1ms)
void Buzz_Play(void)
{
	enable_buzz = 1;
}

// TODO: Use Timer_IRQ instead of HAL_Delay
void Buzz_AddNote(notes_t note, uint32_t duration_ms)
{	
	if (melody.sound_idx >= SOUND_BUF_SIZE)
	{
		errmsg("Sound buffer overflow\r\n");
		melody.sound_idx = 0;
	}
	melody.sound[melody.sound_idx].note = note;
	melody.sound[melody.sound_idx].dur = duration_ms;
	melody.sound[melody.sound_idx].act = 1;
	melody.sound_idx++;
}

void Buzz_PlayMario(void)
{
	Buzz_SetTemp(90);
	ResetSoundBuf();
	// Intro
	Buzz_AddNote(E1, ndSIXT);
	Buzz_AddNote(E1, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(E1, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(C1, ndSIXT);
	Buzz_AddNote(E1, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(G1, ndSIXT + ndTWSEC);
	Buzz_AddNote(P, ndSIXT + ndEIGHT);
	Buzz_AddNote(G0, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	// Part A.1
	Buzz_AddNote(P, ndEIGHT);
	Buzz_AddNote(C1, ndSIXT);
	Buzz_AddNote(P, ndEIGHT);
	Buzz_AddNote(G0, ndSIXT);
	Buzz_AddNote(P, ndEIGHT);
	Buzz_AddNote(E0, ndSIXT);
	Buzz_AddNote(P, ndEIGHT);
	
	Buzz_AddNote(A0, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(B0, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(A0dies, ndSIXT);
	Buzz_AddNote(A0, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(G0, ndSIXT);
	Buzz_AddNote(E1, 1.5*ndSIXT);
	Buzz_AddNote(P, ndSIXT/2);
	Buzz_AddNote(G1, ndSIXT);
	Buzz_AddNote(A1, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(F1, ndSIXT);
	Buzz_AddNote(G1, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(E1, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(C1, ndSIXT);
	Buzz_AddNote(D1, ndSIXT);
	Buzz_AddNote(B0, ndSIXT);	
	// Part A.2
	Buzz_AddNote(P, ndEIGHT);
	Buzz_AddNote(C1, ndSIXT);
	Buzz_AddNote(P, ndEIGHT);
	Buzz_AddNote(G0, ndSIXT);
	Buzz_AddNote(P, ndEIGHT);
	Buzz_AddNote(E0, ndSIXT);
	Buzz_AddNote(P, ndEIGHT);

	Buzz_AddNote(A0, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(B0, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(A0dies, ndSIXT);
	Buzz_AddNote(A0, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(G0, ndSIXT);
	Buzz_AddNote(E1, 1.5*ndSIXT);
	Buzz_AddNote(P, ndSIXT/2);
	Buzz_AddNote(G1, ndSIXT);
	Buzz_AddNote(A1, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(F1, ndSIXT);
	Buzz_AddNote(G1, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(E1, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(C1, ndSIXT);
	Buzz_AddNote(D1, ndSIXT);
	Buzz_AddNote(B0, ndSIXT);	
	// Part B
	Buzz_AddNote(P, ndQUART);
	Buzz_AddNote(G1, ndSIXT);
	Buzz_AddNote(F1dies, ndSIXT);
	Buzz_AddNote(F1, ndSIXT);
	Buzz_AddNote(D1, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(E1, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(G0, ndSIXT);
	Buzz_AddNote(A0, ndSIXT);
	Buzz_AddNote(C1, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(A0, ndSIXT);
	Buzz_AddNote(C1, ndSIXT);
	Buzz_AddNote(D1, ndSIXT);
	Buzz_AddNote(P, ndEIGHT);
	Buzz_AddNote(G1, ndSIXT);
	Buzz_AddNote(F1dies, ndSIXT);
	Buzz_AddNote(F1, ndSIXT);
	Buzz_AddNote(D1, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(E1, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(C2, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(C2, ndSIXT);
	Buzz_AddNote(C2, ndEIGHT);
	Buzz_AddNote(P, ndQUART);
	
	Buzz_AddNote(G1, ndSIXT);
	Buzz_AddNote(F1dies, ndSIXT);
	Buzz_AddNote(F1, ndSIXT);
	Buzz_AddNote(D1, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(E1, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(G0, ndSIXT);
	Buzz_AddNote(A0, ndSIXT);
	Buzz_AddNote(C1, ndSIXT);
	Buzz_AddNote(P, ndSIXT);
	Buzz_AddNote(A0, ndSIXT);
	Buzz_AddNote(C1, ndSIXT);
	Buzz_AddNote(D1, ndSIXT);
	Buzz_AddNote(P, ndEIGHT);
	Buzz_AddNote(D1dies, ndSIXT);
	Buzz_AddNote(P, ndEIGHT);
	Buzz_AddNote(D1, ndSIXT);
	Buzz_AddNote(P, ndEIGHT);
	Buzz_AddNote(C1, ndSIXT);

	Buzz_Play();
}

void Buzz_PlayClintEastwood(void)
{
	ResetSoundBuf();
	
	Buzz_AddNote(E1, ndSIXT);
	Buzz_AddNote(D1, ndSIXT);
	Buzz_AddNote(E1, ndSIXT);
	Buzz_AddNote(B0, ndSIXT);
	
	Buzz_AddNote(E0, ndSIXT);
	Buzz_AddNote(E0, ndSIXT);
	Buzz_AddNote(G0, ndSIXT);
	Buzz_AddNote(B0, ndSIXT);
	
	Buzz_AddNote(A0, ndSIXT);
	Buzz_AddNote(B0, ndSIXT);
	Buzz_AddNote(E1, ndSIXT);
	Buzz_AddNote(B0, ndSIXT);
	Buzz_AddNote(E0, ndSIXT);
	Buzz_AddNote(G0, ndSIXT);
	Buzz_AddNote(B0, ndSIXT);
	
	Buzz_AddNote(A0, ndSIXT);
	Buzz_AddNote(B0, ndSIXT);
	Buzz_AddNote(B0, ndSIXT);
	Buzz_AddNote(B0, ndSIXT);
	Buzz_AddNote(G0, ndSIXT);
	Buzz_AddNote(A0, ndSIXT);
	Buzz_AddNote(G0, ndSIXT);
	Buzz_AddNote(A0, ndSIXT);
	
	Buzz_AddNote(B0, ndSIXT);
	Buzz_AddNote(G0, ndSIXT);
	Buzz_AddNote(E0, ndSIXT);
	Buzz_AddNote(E0, ndSIXT);
	Buzz_AddNote(D0, ndSIXT);
	Buzz_AddNote(E0, ndSIXT);
	Buzz_AddNote(E0, ndSIXT);
	
	Buzz_Play();
}


