/**
  ******************************************************************************
  * @file           : leds.h
  * @brief          : Header of Module working with CNCU leds
  ******************************************************************************
*/

#ifndef _CNCU_LEDS_H
#define _CNCU_LEDS_H
  
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "cncu_conf.h"
  
/* Private define ------------------------------------------------------------*/
#define ERR_LED_Pin						GPIO_PIN_3
#define CAN_LED_Pin						GPIO_PIN_4
#define GEN_LED_Pin						GPIO_PIN_5

#define RUNNING_BLINK_PERIOD	100		// [ms]
#define NUMBER_OF_LEDS				3

/* Exported types ------------------------------------------------------------*/
// Led types enum
  typedef enum
  {
		ERR_LED = 2,					// Error occurrence LED
		CAN_LED,							// CAN exchange LED
		GEN_LED,							// General purpose LED
		ALL
  }ELEDS;

// Leds options structure 
	typedef struct
	{
		ELEDS type;
		_Bool blink_en;
		uint32_t blink_period;
		volatile uint32_t blink_cnt;
	}Led_t;
	
/* Exported variables --------------------------------------------------------*/
extern TIM_HandleTypeDef xTIM11;	// TIM11 Handle
extern volatile ELEDS led_cnt;		// For running blink
  
/* Exported constants --------------------------------------------------------*/
  
/* Exported functions --------------------------------------------------------*/
/**
  * @brief 	Initializing GPIO for toggling onboard leds   
  * @param	None
  * @retval	None  
 */
uint8_t Leds_Init (void);
  
/**
  * @brief 	Set LED    
  * @param	ltype - type of led to be set (from enum)
  * @retval	None  
 */
void Leds_ON (ELEDS ltype);
  
/**
  * @brief 	Reset LED   
  * @param	ltype - type of led to be set (from enum)
  * @retval	None  
 */
void Leds_OFF (ELEDS ltype);
  
/**
  * @brief 	Toggle LED   
  * @param	ltype - type of led to be set (from enum)
  * @retval	None  
 */
void Leds_Toggle (ELEDS ltype);

/**
	@brief Start 
 */
void Leds_StartRunningBlink (void);

/**
	@brief Stops TIM11 blinking leds one by one
 */
void Leds_StopRunningBlink (void);

/**
  * @brief 	Start self blink led with determined period   
  * @param	ltype - type of led to be blink
						period
  * @retval	None  
 */
void Leds_StartBlink (ELEDS ltype, uint32_t period);

/**
  * @brief 	Stop self blink led   
  * @param	ltype - type of led to stop blinking
  * @retval	None  
 */
void Leds_StopBlink (ELEDS ltype);

void Leds_ISR (void);

#endif	//_CNCU_LEDS_H
