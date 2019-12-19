/**
  ******************************************************************************
  * @file           : can.h
  * @brief          : Header of Module working with CNCU CAN 2.0B
  ******************************************************************************
*/

#ifndef _CNCU_I2C_H
#define _CNCU_I2C_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "cncu_conf.h"

/* Exported types ------------------------------------------------------------*/
extern CAN_HandleTypeDef xCAN1;

/* Exported constants --------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/ 
/**
  * @brief 	Initialization of I2C onboard interface.  
  * @param	
  * @retval	0 - SUCCESS
						1 - Error
 */ 
char I2C_Init(void);
#endif // _CNCU_I2C_H
