/**
  ******************************************************************************
  * @file           : uart.c
  * @brief          : Module working with CNCU UART\USART interface
										CNCU-01 Board supports 3 USART interfaces:
										- USART1 <-> USB-Serial (FTDI230XS)
										- USART2 <-> RS232 or RS485
										- USART3 <-> TTL UART 
  ******************************************************************************
*/
#include "uart.h"
#include "FIFO.h"

#define DEBUG_MSG DEBUG_UART
#include "dbgmsg.h"

FIFO_CREATE(USART1_fifo, char, USART1_FIFO_SIZE);
FIFO_CREATE(USART3_fifo, char, USART3_FIFO_SIZE);
char CONSOLE_BUF[USART1_FIFO_SIZE];		// Buffer for console messages (strings)
UART_HandleTypeDef xUSART1;						// Structure for USART1 config
UART_HandleTypeDef xUSART3;

/* Private functions (HAL_Init callback)--------------------------------------*/

void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct;
	
  if(huart->Instance==USART1)
  {
    /* Peripheral clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();
  
    /**USART1 GPIO Configuration    
    PA9     ------> USART1_TX
    PA10    ------> USART1_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
  else if(huart->Instance==USART2)
  {
    /* Peripheral clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();
  
    /**USART2 GPIO Configuration (RS232\485)    
    PD5     ------> USART2_TX   
    PD6     ------> USART2_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
  }
  else if(huart->Instance==USART3)
  {
    /* Peripheral clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();
  
    /**USART3 GPIO Configuration    
    PD8     ------> USART3_TX
    PD9     ------> USART3_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{

  if(huart->Instance==USART1)
  {
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();
  
    /**USART1 GPIO Configuration    
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX 
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);
  }
  else if(huart->Instance==USART2)
  {
    /* Peripheral clock disable */
    __HAL_RCC_USART2_CLK_DISABLE();
  
    /**USART2 GPIO Configuration    
    PD5     ------> USART2_TX
    PD6     ------> USART2_RX 
    */
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_5|GPIO_PIN_6);
  }
  else if(huart->Instance==USART3)
  {
    /* Peripheral clock disable */
    __HAL_RCC_USART3_CLK_DISABLE();
  
    /**USART3 GPIO Configuration    
    PD8     ------> USART3_TX
    PD9     ------> USART3_RX 
    */
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_8|GPIO_PIN_9);
  }
}

/*******************************************************************************
                          Module interface (Common functions) 
*******************************************************************************/
/* USART1 init function */
cncu_err_t USART1_Init(uint32_t bdr)
{
	xUSART1.Instance = USART1;
	xUSART1.Init.BaudRate = bdr;
	xUSART1.Init.WordLength = UART_WORDLENGTH_8B;
	xUSART1.Init.StopBits = UART_STOPBITS_1;
	xUSART1.Init.Parity = UART_PARITY_NONE;
	xUSART1.Init.Mode = UART_MODE_TX_RX;
	xUSART1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	xUSART1.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&xUSART1) != HAL_OK)
	{
		//errmsg("UART: Unable to init USART1\r\n");
		return INIT_ERR;
	}
	/* Enable specified interrupts !! */
	//SET_BIT(xUSART1.Instance->CR1,USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE);
	__HAL_UART_ENABLE_IT(&xUSART1, UART_IT_RXNE);
	/* USART1_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(USART1_IRQn, USART1_PreP, USART1_SubP);
	HAL_NVIC_EnableIRQ(USART1_IRQn);
	
	FIFO_FLUSH(USART1_fifo);

	return CNCU_OK;
}

/* USART3 init function */
cncu_err_t USART3_Init(uint32_t bdr)
{
	xUSART3.Instance = USART3;
	xUSART3.Init.BaudRate = bdr;
	xUSART3.Init.WordLength = UART_WORDLENGTH_8B;
	xUSART3.Init.StopBits = UART_STOPBITS_1;
	xUSART3.Init.Parity = UART_PARITY_NONE;
	xUSART3.Init.Mode = UART_MODE_TX_RX;
	xUSART3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	xUSART3.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&xUSART3) != HAL_OK)
	{
		//errmsg("UART: Unable to init USART3\r\n");
		return INIT_ERR;
	}
	__HAL_UART_ENABLE_IT(&xUSART3, UART_IT_RXNE);
	HAL_NVIC_SetPriority(USART3_IRQn, USART3_PreP, USART3_SubP);
	HAL_NVIC_EnableIRQ(USART3_IRQn);

	FIFO_FLUSH(USART3_fifo);

	return CNCU_OK;
}

// Send data array via USART in blocking mode
HAL_StatusTypeDef USART1_Send(uint8_t *pData, uint16_t Size)
{
	return HAL_UART_Transmit(&xUSART1, pData, Size, HAL_MAX_DELAY);
}

HAL_StatusTypeDef USART3_Send(uint8_t *pData, uint16_t Size)
{
	return HAL_UART_Transmit(&xUSART3, pData, Size, HAL_MAX_DELAY);
}

// Non blocking mode
HAL_StatusTypeDef USART3_Send2(uint8_t *pData, uint16_t Size)
{
	while (HAL_UART_GetState(&xUSART3) != HAL_UART_STATE_READY);
	// will overwrite the buffer otherwise, (needs a timeout and error handling)
	return HAL_UART_Transmit_IT(&xUSART3, pData, Size);
}

// Get byte from software FIFO 
HAL_StatusTypeDef USART1_Get (uint8_t *pStore, uint16_t Size)
{
	return HAL_UART_Receive(&xUSART1, pStore, Size, 0xFFFF);
}

HAL_StatusTypeDef USART3_Get (uint8_t *pStore, uint16_t Size)
{
	return HAL_UART_Receive(&xUSART3, pStore, Size, 0xFFFF);
}

// Read all available data from FIFO buffer
// TODO: Link Console to USART1 (Serial USB) or USART3 (UART)
static void ReadConsoleFIFO (char *pstore)
{
	uint32_t cnt = FIFO_AVAIL_COUNT(USART1_fifo);
	if FIFO_IS_FULL(USART1_fifo) dbgmsg("FIFO is full\r\n");
	dbgmsg("\r\ncnt: %d\r\n", cnt);
	for (int i =0; i < cnt; i++)
	{
		FIFO_POP(USART1_fifo, *pstore++);
	}
	// We've read all available data from fifo, so clear it for new input
	FIFO_FLUSH(USART1_fifo);
}

// USB-Serial Interrupt routine (Console)
void USART1_ISR(void)
{
	char tmp = (uint16_t)(xUSART1.Instance->DR & (uint16_t)0x01FF);

	if (tmp != '\r') FIFO_PUSH(USART1_fifo, tmp)
	else 
	{
		ReadConsoleFIFO(CONSOLE_BUF);

		dbgmsg("USART1 echo: %s\r\n", CONSOLE_BUF);
		
    // Release semaphore for console thread for further parsing
		
		memset(CONSOLE_BUF, '\0' , sizeof(CONSOLE_BUF));	// clear buffer
	}
}

// UART Interrupt routine (temporary)
void USART3_ISR(void)
{	
	char tmp = (uint16_t)(xUSART3.Instance->DR & (uint16_t)0x01FF);

	if (tmp != '\r') FIFO_PUSH(USART3_fifo, tmp)
	else 
	{
		ReadConsoleFIFO(CONSOLE_BUF);

		dbgmsg("USART3 echo: %s\r\n", CONSOLE_BUF);
		
    // Release semaphore for console thread for further parsing
		
		memset (CONSOLE_BUF, '\0' , sizeof(CONSOLE_BUF));	// clear buffer
	}
}









