/**
  ******************************************************************************
  * @file           : gtimer.h
  * @brief          : Header of Module working with CNCU GeneralPurpose Timer
  ******************************************************************************
*/

#ifndef _CNCU_GTIM_H
#define _CNCU_GTIM_H
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "cncu_conf.h"
  
/* Private define ------------------------------------------------------------*/  
/* Exported types ------------------------------------------------------------*/
// GeneralPurpose Timer work structure
typedef struct
{
  unsigned act : 1;		// Flag if timer is active (1 - active)
  unsigned tmout : 1;	// Flag if timer reaches period (timeout)
  unsigned RFU : 6;		// Reserved for future use
  uint32_t cnt;				// Gtimer's current counter
}gtimer_t;

// GeneralPurpose Timer resolutions 
typedef enum
{
  ms_resolution,
  us_resolution
}ERESOL;

/* Exported variables --------------------------------------------------------*/
extern TIM_HandleTypeDef xTIM10;		// Gtimer Handle
extern volatile gtimer_t Gtimer;			// Gtimer working structure

/* Exported constants --------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/ 
void Gtimer_ISR(void);

/**
  * @brief 	Initializsation of Gtimer with tick = 1 ms  
  * @param	None
  * @retval	0 - SUCCESS
			1 - FAILURE
 */
cncu_err_t Gtimer_Init (ERESOL resolution);

/**
  * @brief 	Start Gtimer's period-reaching IRQ   
  * @param	timeout - time in ms. If reached = time is out
  * @retval	None  
 */
void Gtimer_Start (void);

/**
  * @brief 	Stop generating period-reaching IRQ   
  * @param	None
  * @retval	None  
 */
void Gtimer_Stop (void);

/**
  * @brief 	Get Gtimer's current counter   
  * @param	None
  * @retval	None  
 */
extern inline uint16_t Gtimer_GetCnt (void);

#endif // _CNCU_GTIM_H
