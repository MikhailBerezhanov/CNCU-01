/**
  ******************************************************************************
  * @file           : spi.h
  * @brief          : Header of Module working with CNCU SPI interface
  ******************************************************************************
*/

#ifndef _CNCU_SPI_H
  #define _CNCU_SPI_H
/* Includes ------------------------------------------------------------------*/
  #include "stm32f4xx_hal.h"
  #include "main.h"
  
/* Exported types ------------------------------------------------------------*/
typedef enum
{
	eSPI1 = 1,
	eSPI3 = 3
}TSPI_NUM;

/* Exported constants --------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
/**
  * @brief 	Initialization of SPI onboard interface   
  * @param	spi_num - hardware port number (CNCU Board supports #1)
  * @retval	0 - OK, 1 - HAL error  
 */  
uint8_t SPI_Init (TSPI_NUM spi_num);

/**
  * @brief 	   
  * @param	spi_num	- -//-
			*pData	- pointer at data buf to be sent,
			Size	- size of data buf in bytes
  * @retval	HAL_OK       = 0x00U,
			HAL_ERROR    = 0x01U,
			HAL_BUSY     = 0x02U,
			HAL_TIMEOUT  = 0x03U  
 */  
HAL_StatusTypeDef SPI_Transmit (TSPI_NUM spi_num, uint8_t *pData, uint16_t Size);

/**
  * @brief 	   
  * @param	spi_num	- -//-
			*pData	- pointer at data buf to store data,
			Size	- size of data buf in bytes to read
  * @retval	HAL_OK       = 0x00U,
			HAL_ERROR    = 0x01U,
			HAL_BUSY     = 0x02U,
			HAL_TIMEOUT  = 0x03U  
 */
HAL_StatusTypeDef SPI_Receive (TSPI_NUM spi_num, uint8_t *pData, uint16_t Size);

/**
  * @brief 	   
  * @param	spi_num	- -//-
			*pTxData- pointer at data buf to be sent,
			*pRxData- pointer at data buf to store data,
			Size	- size of data buf in bytes to read
  * @retval	HAL_OK       = 0x00U,
		    HAL_ERROR    = 0x01U,
		    HAL_BUSY     = 0x02U,
		    HAL_TIMEOUT  = 0x03U  
 */
HAL_StatusTypeDef SPI_Transceive (TSPI_NUM spi_num, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size);

#endif // _CNCU_SPI_H
