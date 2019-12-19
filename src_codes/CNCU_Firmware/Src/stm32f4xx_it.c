/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  *
  * COPYRIGHT(c) 2018 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "stm32f4xx.h"
#include "stm32f4xx_it.h"
#include "cmsis_os.h"
#include "leds.h"
#include "ethernetif.h"
#include "i2c.h"
#include "uart.h"
#include "can.h"
#include "gtimer.h"
#include "buzz.h"
#include "sdcard.h"

#define DEBUG_MSG 1
#include "dbgmsg.h"

/* External variables --------------------------------------------------------*/


/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/**
* @brief This function handles USART1 global interrupt.
*/
void USART1_IRQHandler(void)
{
  //HAL_UART_IRQHandler(&xUSART1);
	
	if (__HAL_UART_GET_FLAG(&xUSART1, UART_FLAG_RXNE) != RESET)
	{
		if (__HAL_UART_GET_IT_SOURCE(&xUSART1, UART_IT_RXNE) != RESET)
		{
			USART1_ISR();
     	__HAL_UART_CLEAR_FLAG(&xUSART1, UART_FLAG_RXNE);
		}
	}
}

/**
* @brief This function handles USART3 global interrupt.
*/
void USART3_IRQHandler(void)
{
	//HAL_UART_IRQHandler(&xUSART3);
	
	if (__HAL_UART_GET_FLAG(&xUSART3, UART_FLAG_RXNE) != RESET)
	{
		if (__HAL_UART_GET_IT_SOURCE(&xUSART3, UART_IT_RXNE) != RESET)
		{
			USART3_ISR();
			__HAL_UART_CLEAR_FLAG(&xUSART3, UART_FLAG_RXNE);
		}
	}
}

/**
* @brief This function handles Ethernet global interrupt.
*/
void ETH_IRQHandler(void)
{
  HAL_ETH_IRQHandler(&heth);
}

/**
* @brief This function handles CAN1 RX0 interrupts.
*/
void CAN1_RX0_IRQHandler(void)
{
  HAL_CAN_IRQHandler(&xCAN1);
}

/**
* @brief This function handles TIM1 trigger and commutation interrupts and TIM11 global interrupt.
* @note	 CNCU Led blinking purposes is handled here.
*/
void TIM1_TRG_COM_TIM11_IRQHandler(void)
{
  //HAL_TIM_IRQHandler(&xTIM11);
  if(__HAL_TIM_GET_FLAG(&xTIM11, TIM_FLAG_UPDATE) != RESET)
  {
    if(__HAL_TIM_GET_IT_SOURCE(&xTIM11, TIM_IT_UPDATE) != RESET)
    {
			Leds_ISR();
      __HAL_TIM_CLEAR_IT(&xTIM11, TIM_IT_UPDATE);
    }
  }
}

/**
* @brief This function handles TIM1 update interrupt and TIM10 global interrupt.
* @note	 CNCU GeneralPurpose Timer is handled here.
*/
void TIM1_UP_TIM10_IRQHandler(void)
{
  //HAL_TIM_IRQHandler(&xTIM10);
  if(__HAL_TIM_GET_FLAG(&xTIM10, TIM_FLAG_UPDATE) != RESET)
  {
    if(__HAL_TIM_GET_IT_SOURCE(&xTIM10, TIM_IT_UPDATE) != RESET)
    {
			Gtimer_ISR();
      __HAL_TIM_CLEAR_IT(&xTIM10, TIM_IT_UPDATE);
    }	
  }
}

/**
  * @brief This function handles SDIO global interrupt.
  */
void SDIO_IRQHandler(void)
{
  HAL_SD_IRQHandler(&hsd);
}

/**
  * @brief This function handles DMA2 stream3 global interrupt.
  */
void DMA2_Stream3_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_sdio_rx);
}

/**
  * @brief This function handles DMA2 stream6 global interrupt.
  */
void DMA2_Stream6_IRQHandler(void)
{
  HAL_DMA_IRQHandler(&hdma_sdio_tx);
}

/******************************************************************************/
/*            Cortex-M4 Processor Interruption and Exception Handlers         */ 
/******************************************************************************/

/**
* @brief This function handles System tick timer.
*/
void SysTick_Handler(void)
{
	BuzzTIM_ISR();
  HAL_IncTick();
  osSystickHandler();	
}

/**
* @brief This function handles Non maskable interrupt.
*/
void NMI_Handler(void)
{

}

/**
* @brief This function handles Hard fault interrupt. (Stack Overflow)
*/
void HardFault_Handler(void)
{

	//if (!CNCU.Periph.USART1status)
	errmsg("EXCEPTION: Hard Fault Interrupt\r\n");
	
  while (1)
  {
	  
  }

}

/**
* @brief This function handles Memory management fault.
*/
void MemManage_Handler(void)
{

	//if (!CNCU.Periph.USART1status)
	errmsg("EXCEPTION: Memory Management Fault\r\n");
	
  while (1)
  {

  }

}

/**
* @brief This function handles Pre-fetch fault, memory access fault.
*/
void BusFault_Handler(void)
{

	//if (!CNCU.Periph.USART1status)
	errmsg("EXCEPTION: Memory Access Fault\r\n");
	
  while (1)
  {

  }

}

/**
* @brief This function handles Undefined instruction or illegal state.
*/
void UsageFault_Handler(void)
{

	//if (!CNCU.Periph.USART1status)
	errmsg("EXCEPTION: Undefiend Insruction or Illegal State\r\n");
	
  while (1)
  {

  }

}

/**
* @brief This function handles Debug monitor.
*/
void DebugMon_Handler(void)
{

}

