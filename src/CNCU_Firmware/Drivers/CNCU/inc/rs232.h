/**
  ******************************************************************************
  * @file           : rs232.h
  * @brief          : Header of Module working with CNCU RS232 interface
  ******************************************************************************
*/

#ifndef _CNCU_RS232_H
#define _CNCU_RS232_H
  
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "cncu_conf.h"
  
/* Private define ------------------------------------------------------------*/  
/* Exported types ------------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
extern UART_HandleTypeDef xUSART2;

/* Exported constants --------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/  
/**
  * @brief 	Initialization of RS232 onboard interface   
  * @param	baudr - baudrate
  * @retval	0 - SUCCESS 1 - ERROR  
 */  
cncu_err_t RS232_Init (uint32_t baudr); 

/**
  * @brief 	Sendind byte via onboard RS232 (blocking mode with timeout)   
  * @param	byte - value to send
  * @retval	Result of execution: HAL_OK - SUCCESS, HAL_ERROR - FAILURE
 */  
HAL_StatusTypeDef RS232_SendByte (uint8_t byte);

/**
  * @brief 	Getting byte from onboard RS232 (blocking mode with timeout)   
  * @param	*byte - buf to store received value
  * @retval Result of execution: HAL_OK - SUCCESS, HAL_ERROR - FAILURE 
 */  
HAL_StatusTypeDef RS232_GetByte (uint8_t *byte);

#endif // _CNCU_RS232_H
