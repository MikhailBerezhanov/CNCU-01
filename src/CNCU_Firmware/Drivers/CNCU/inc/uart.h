/**
  ******************************************************************************
  * @file           : uart.h
  * @brief          : Header of Module working with CNCU UART\USART interfaces
  ******************************************************************************
*/

#ifndef _CNCU_UART_H
#define _CNCU_UART_H

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "cncu_conf.h"

/* Private define ------------------------------------------------------------*/
#define USART1_FIFO_SIZE      64
#define	USART3_FIFO_SIZE			7

/* Exported types ------------------------------------------------------------*/
extern UART_HandleTypeDef xUSART1;
extern UART_HandleTypeDef xUSART3;

/* Exported constants --------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/ 
/**
  * @brief 	Initialization of UART onboard interface.  
  * @param	bdr - UART baudrate (up to 2 Mbps).
  * @retval	0 - SUCCESS
						1 - Error
 */ 
cncu_err_t USART1_Init(uint32_t bdr);
cncu_err_t USART3_Init(uint32_t bdr);

/**
  * @brief 	Sending data frame via CAN
  * @note	Using Extended ID format.  
  * @param	id 			- Frame's ID
			dlc 		- Frame's data length
			aData 		- pointer at data buf to send
  * @retval	HAL_OK 		- SUCCESS
			HAL_ERROR 	- No free Tx buffers
 */ 
HAL_StatusTypeDef USART1_Send(uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef USART3_Send(uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef USART3_Send2(uint8_t *pData, uint16_t Size);

HAL_StatusTypeDef USART1_Get (uint8_t *pStore, uint16_t Size);
HAL_StatusTypeDef USART3_Get (uint8_t *pStore, uint16_t Size);


void USART1_ISR(void);
void USART3_ISR(void);

#endif // _CNCU_UART_H
