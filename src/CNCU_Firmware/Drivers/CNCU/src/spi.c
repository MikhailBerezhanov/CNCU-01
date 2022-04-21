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
}

/*******************************************************************************
                          Module interface (Common functions) 
*******************************************************************************/
/**
	@brief Calculating prescaler value depending on PCLK frequency 
*/
static uint32_t SPI_CalculatePrescaler (uint32_t freq) 
{
	uint32_t pclk = HAL_RCC_GetPCLK2Freq();	// MAX 84 MHz
	// MIN Prescaler is 2 so MAX SPI CLK is 42 MHz
	
	uint32_t div = pclk / freq;
	
	if (div < 4) return SPI_BAUDRATEPRESCALER_2;
	else if (div < 8) return SPI_BAUDRATEPRESCALER_4;
	else if (div < 16) return SPI_BAUDRATEPRESCALER_8;
	else if (div < 32) return SPI_BAUDRATEPRESCALER_16;
	else if (div < 64) return SPI_BAUDRATEPRESCALER_32;
	else if (div < 128) return SPI_BAUDRATEPRESCALER_64;
	else if (div < 256) return SPI_BAUDRATEPRESCALER_128;
	else return SPI_BAUDRATEPRESCALER_256;
	
}

static void SPI_PrintSCKValue (uint32_t prescaler)
{
	uint32_t dec_presc = 1;
	
	switch (prescaler)
	{
		case SPI_BAUDRATEPRESCALER_2: dec_presc = 2; break;
		case SPI_BAUDRATEPRESCALER_4: dec_presc = 4; break;
		case SPI_BAUDRATEPRESCALER_8: dec_presc = 8; break;
		case SPI_BAUDRATEPRESCALER_16: dec_presc = 16; break;
		case SPI_BAUDRATEPRESCALER_32: dec_presc = 32; break;
		case SPI_BAUDRATEPRESCALER_64: dec_presc = 64; break;
		case SPI_BAUDRATEPRESCALER_128: dec_presc = 128; break;
		case SPI_BAUDRATEPRESCALER_256: dec_presc = 256; break;
		default: errmsg("Unkown SPI prescaler\r\n");
	}
	
	dbgmsg("SPI Prescaler: %d, SPI SCK: %d [Hz]\r\n", dec_presc, HAL_RCC_GetPCLK2Freq() / dec_presc);
}

/**
	@brief Initialization of SPI onboard interface
	@note: EEPROM supports up to 16 MHz, FLASH supports up to 85 MHz
 */ 
cncu_err_t SPI_Init (uint32_t freq)
{
	/* SPI1 parameter configuration. SPI1 is on APB2 CLK bus (PCLK2) */
	xSPI1.Instance = SPI1;						
	xSPI1.Init.Mode = SPI_MODE_MASTER;
	xSPI1.Init.Direction = SPI_DIRECTION_2LINES;
	xSPI1.Init.DataSize = SPI_DATASIZE_8BIT;
	xSPI1.Init.CLKPolarity = SPI_POLARITY_LOW;
	xSPI1.Init.CLKPhase = SPI_PHASE_1EDGE;
	xSPI1.Init.NSS = SPI_NSS_SOFT;
	xSPI1.Init.BaudRatePrescaler = SPI_CalculatePrescaler(freq);//SPI_BAUDRATEPRESCALER_16;   // 5.25 MHz
	SPI_PrintSCKValue(xSPI1.Init.BaudRatePrescaler);
	xSPI1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	xSPI1.Init.TIMode = SPI_TIMODE_DISABLE;
	xSPI1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	xSPI1.Init.CRCPolynomial = 10;
	if (HAL_SPI_Init(&xSPI1) != HAL_OK)
	{
		errmsg("SPI: Unable to init SPI1\r\n");
		return INIT_ERR;
	}
	
	return CNCU_OK;  
}

/**
	@brief Sending data via SPI (blocking mode with timeout)
 */ 
HAL_StatusTypeDef SPI_Transmit (uint8_t *pData, uint16_t Size)
{
	return HAL_SPI_Transmit(&xSPI1, pData, Size, 0xFFFF);
}

/**
	@brief Getting data via SPI (blocking mode with timeout)
 */ 
HAL_StatusTypeDef SPI_Receive (uint8_t *pData, uint16_t Size)
{
	return HAL_SPI_Receive(&xSPI1, pData, Size, 0xFFFF);
}

/**
	@brief Getting data via SPI (blocking mode with timeout)
 */ 
HAL_StatusTypeDef SPI_Transceive (uint8_t *pTxData, uint8_t *pRxData, uint16_t Size)
{
	return HAL_SPI_TransmitReceive(&xSPI1, pTxData, pRxData, Size, 0xFFFF);
}
