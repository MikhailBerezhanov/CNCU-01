/**
  ******************************************************************************
  * @file           : spi.c
  * @brief          : Module working with CNCU SPI interfaces
  ******************************************************************************
*/
#include "spi.h"

#define DEBUG_MSG DEBUG_SPI
#include "dbgmsg.h"

static SPI_HandleTypeDef xSPI1;						// SPI1 Handle
static SPI_HandleTypeDef xSPI3;						// SPI3 Handle

/* Private functions ---------------------------------------------------------*/

void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(hspi->Instance==SPI1)
  {
    /* Peripheral clock enable */
    __HAL_RCC_SPI1_CLK_ENABLE();
  
    /**SPI1 GPIO Configuration    
    PA5     ------> SPI1_SCK
    PA6     ------> SPI1_MISO
    PB5     ------> SPI1_MOSI 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }
  else if(hspi->Instance==SPI3)
  {
    /* Peripheral clock enable */
    __HAL_RCC_SPI3_CLK_ENABLE();
  
    /**SPI3 GPIO Configuration    
    PC10     ------> SPI3_SCK
    PC11     ------> SPI3_MISO
    PC12     ------> SPI3_MOSI 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi)
{
  if(hspi->Instance==SPI1)
  {
    /* Peripheral clock disable */
    __HAL_RCC_SPI1_CLK_DISABLE();
  
    /**SPI1 GPIO Configuration    
    PA5     ------> SPI1_SCK
    PA6     ------> SPI1_MISO
    PB5     ------> SPI1_MOSI 
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5|GPIO_PIN_6);

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_5);
  }
  else if(hspi->Instance==SPI3)
  {
    /* Peripheral clock disable */
    __HAL_RCC_SPI3_CLK_DISABLE();
  
    /**SPI3 GPIO Configuration    
    PC10     ------> SPI3_SCK
    PC11     ------> SPI3_MISO
    PC12     ------> SPI3_MOSI 
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12);
  }
}

/*******************************************************************************
                          Module interface (Common functions) 
*******************************************************************************/
/**
	@brief Initialization of SPI onboard interface
 */ 
uint8_t SPI_Init (TSPI_NUM spi_num)
{
  switch(spi_num)
  {
	  case eSPI1:  
		/* SPI1 parameter configuration. SPI1 is on APB2 CLK bus (PCLK2) */
		xSPI1.Instance = SPI1;						
		xSPI1.Init.Mode = SPI_MODE_MASTER;
		xSPI1.Init.Direction = SPI_DIRECTION_2LINES;
		xSPI1.Init.DataSize = SPI_DATASIZE_8BIT;
		xSPI1.Init.CLKPolarity = SPI_POLARITY_LOW;
		xSPI1.Init.CLKPhase = SPI_PHASE_1EDGE;
		xSPI1.Init.NSS = SPI_NSS_SOFT;
		// NOTE: EEPROM supports up to 16 MHz, FLASH supports up to 85 MHz
		xSPI1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;   // 5.25 MHz
		xSPI1.Init.FirstBit = SPI_FIRSTBIT_MSB;
		xSPI1.Init.TIMode = SPI_TIMODE_DISABLE;
		xSPI1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
		xSPI1.Init.CRCPolynomial = 10;
		if (HAL_SPI_Init(&xSPI1) != HAL_OK)
		{
			errmsg("SPI: Unable to init SPI1\r\n");
			return 1;
		}
		return 0;
		
	  case eSPI3:
		/* SPI3 parameter configuration. SPI3 is on APB1 CLK bus (PCLK1)*/
		xSPI3.Instance = SPI3;
		xSPI3.Init.Mode = SPI_MODE_MASTER;
		xSPI3.Init.Direction = SPI_DIRECTION_2LINES;
		xSPI3.Init.DataSize = SPI_DATASIZE_8BIT;
		xSPI3.Init.CLKPolarity = SPI_POLARITY_LOW;
		xSPI3.Init.CLKPhase = SPI_PHASE_1EDGE;
		xSPI3.Init.NSS = SPI_NSS_SOFT;
		xSPI3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
		xSPI3.Init.FirstBit = SPI_FIRSTBIT_MSB;
		xSPI3.Init.TIMode = SPI_TIMODE_DISABLE;
		xSPI3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
		xSPI3.Init.CRCPolynomial = 10;
		if (HAL_SPI_Init(&xSPI3) != HAL_OK)
		{
			errmsg("SPI: Unable to init SPI3\r\n");
			return 1;
		}
		return 0;  
		
	  default: return 0xFF;
  }
}

/**
	@brief Calculating prescaler value depending on PCLK frequency
  
static uint16_t SPI_CalculatePrescaler (uint32_t freq)
{
	
}*/

/**
	@brief Sending data via SPI (blocking mode with timeout)
 */ 
HAL_StatusTypeDef SPI_Transmit (TSPI_NUM spi_num, uint8_t *pData, uint16_t Size)
{
	switch (spi_num)
	{
		case eSPI1:
			return HAL_SPI_Transmit(&xSPI1, pData, Size, 0xFFFF);
		
		case eSPI3:
			return HAL_SPI_Transmit(&xSPI3, pData, Size, 0xFFFF);
		
		default: return HAL_ERROR;
	}
}

/**
	@brief Getting data via SPI (blocking mode with timeout)
 */ 
HAL_StatusTypeDef SPI_Receive (TSPI_NUM spi_num, uint8_t *pData, uint16_t Size)
{
	switch (spi_num)
	{
		case eSPI1:
			return HAL_SPI_Receive(&xSPI1, pData, Size, 0xFFFF);
		
		case eSPI3:
			return HAL_SPI_Receive(&xSPI3, pData, Size, 0xFFFF);
		
		default: return HAL_ERROR;
	}	
}

/**
	@brief Getting data via SPI (blocking mode with timeout)
 */ 
HAL_StatusTypeDef SPI_Transceive (TSPI_NUM spi_num, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size)
{
	switch (spi_num)
	{
		case eSPI1:
			return HAL_SPI_TransmitReceive(&xSPI1, pTxData, pRxData, Size, 0xFFFF);
		
		case eSPI3:
			return HAL_SPI_TransmitReceive(&xSPI3, pTxData, pRxData, Size, 0xFFFF);

		default: return HAL_ERROR;
	}
}
