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
#include "cncu_conf.h"
  
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
/**
  * @brief 	Initialization of SPI onboard interface   
  * @param	freq - desired SPI SCK
  * @retval	CNCU_OK - Success, INIT_ERR - Failure  
 */  
cncu_err_t SPI_Init (uint32_t freq);

/**
  * @brief 	   
  * @param	
			*pData	- pointer at data buf to be sent,
			Size	- size of data buf in bytes
  * @retval	HAL_OK       = 0x00U,
			HAL_ERROR    = 0x01U,
			HAL_BUSY     = 0x02U,
			HAL_TIMEOUT  = 0x03U  
 */  
HAL_StatusTypeDef SPI_Transmit (uint8_t *pData, uint16_t Size);

/**
  * @brief 	   
  * @param	
			*pData	- pointer at data buf to store data,
			Size	- size of data buf in bytes to read
  * @retval	HAL_OK       = 0x00U,
			HAL_ERROR    = 0x01U,
			HAL_BUSY     = 0x02U,
			HAL_TIMEOUT  = 0x03U  
 */
HAL_StatusTypeDef SPI_Receive (uint8_t *pData, uint16_t Size);

/**
  * @brief 	   
  * @param	
			*pTxData- pointer at data buf to be sent,
			*pRxData- pointer at data buf to store data,
			Size	- size of data buf in bytes to read
  * @retval	HAL_OK       = 0x00U,
		   HAL_ERROR    = 0x01U,
		   HAL_BUSY     = 0x02U,
		   HAL_TIMEOUT  = 0x03U  
 */
HAL_StatusTypeDef SPI_Transceive (uint8_t *pTxData, uint8_t *pRxData, uint16_t Size);

#endif // _CNCU_SPI_H
