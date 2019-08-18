/**
  ******************************************************************************
  * @file           : rs232.c
  * @brief          : Module for work with CNCU RS232 interface
  ******************************************************************************
*/

#include "rs232.h"

#define DEBUG_MSG DEBUG_RS232_485
#include "dbgmsg.h"

UART_HandleTypeDef xUSART2;

/**
	@brief Initialization of RS232 onboard interface
 */ 
uint8_t RS232_Init (uint32_t baudr)
{
  xUSART2.Instance = USART2;
  xUSART2.Init.BaudRate = baudr;
  xUSART2.Init.WordLength = UART_WORDLENGTH_8B;
  xUSART2.Init.StopBits = UART_STOPBITS_1;
  xUSART2.Init.Parity = UART_PARITY_NONE;
  xUSART2.Init.Mode = UART_MODE_TX_RX;
  xUSART2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  xUSART2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&xUSART2) != HAL_OK)
  {
	return 1;
  }
	
  return 0;
}

/**
	@brief Sendind byte via onboard RS232 (blocking mode with timeout)
 */ 
HAL_StatusTypeDef RS232_SendByte (uint8_t byte)
{
  return HAL_UART_Transmit(&xUSART2, (uint8_t *)&byte, 1, 0xFFFF);	
}

/**
	@brief Getting byte from onboard RS232 (blocking mode with timeout)
 */ 
HAL_StatusTypeDef RS232_GetByte (uint8_t *byte)
{
  return HAL_UART_Receive(&xUSART2, byte, 1, 0xFFFF);
}







