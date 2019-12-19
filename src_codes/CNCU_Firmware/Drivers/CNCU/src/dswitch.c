/**
  ******************************************************************************
  * @file           : dwitch.c
  * @brief          : Module for reading CNCU DIPSWITCHs
  ******************************************************************************
*/

#include "dswitch.h"

/**
	@brief GPIO initialization for using DIP switch and reading iput values
 */
uint8_t Get_DSwitch (void)
{
  uint8_t res = 0;
  GPIO_InitTypeDef GPIO_InitStruct;	
	
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();	
	
  /* Configure GPIO pins : PD10 PD11 PD12 PD13 PD14 PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 
                          | GPIO_PIN_14 | GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* Configure GPIO pins : PC6 PC7 */
  GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	
  // getting value from DIP switch
  res = HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_10);				// switch8 - LSB
  res |= HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_11) << 1;			// switch7
  res |= HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_12) << 2;			// switch6
  res |= HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_13) << 3;			// switch5
  res |= HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_14) << 4;			// switch4
  res |= HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_15) << 5;			// switch3
  res |= HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_6)  << 6;			// switch2
  res |= HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_7)  << 7;			// switch1 - MSB
  
  return res;
}
