/**
  ******************************************************************************
  * @file           : can.h
  * @brief          : Header of Module working with CNCU CAN 2.0B
  ******************************************************************************
*/

#ifndef _CNCU_CAN_H
  #define _CNCU_CAN_H
/* Includes ------------------------------------------------------------------*/
  #include "stm32f4xx_hal.h"
  #include "main.h"

	#define CAN_SP	(unsigned long)	70		// [%] Sample point (bit sampling) 
	#define SELFTEST_TMOUT					20		// [ms] Waiting Test Message in RX FIFO
	#define	SENDFRAME_TMOUT					50		// [ms] Trying to put fram into TxMailbox
	#define CAN_FIFO_SIZE						80		// [elements] CAN frames
	
/* Exported types ------------------------------------------------------------*/
extern CAN_HandleTypeDef xCAN1;

typedef __packed struct
{
	unsigned DLC:				4;	// Data lenght (0..8)
	unsigned unused:		2;	
	unsigned RTR:				1;	// 0 - Data frame, 1 - Remote frame
	unsigned IDE:				1;	// 0 - Standart frame, 1 - Extended frame
	uint32_t Header;				// Frame header (11 bit or 29)
	uint8_t Data[8];				// Frame data (max 8 bytes)
}CANframe_t;					// 13 Bytes	
	
// TEMPORARY-------------------
typedef __packed union
{
	__packed struct
	{
	unsigned char PM;
	unsigned SA :	7;
	unsigned DA	:	7;
	unsigned FN	:	7;
	unsigned un	:	3;
	}Field;	
	uint32_t Full;
}CANSKP_ID_t;

typedef __packed struct
{
	CANSKP_ID_t ID;
	unsigned char Data[8];
}CANSKP_MSG_t;
// TEMPORARY-------------------

/* Exported constants --------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/ 
/**
  * @brief 	Initialization of CAN onboard interface.  
  * @param	bdr - CAN baudrate (up to 1 Mbps).
  * @retval	0 - SUCCESS
						-1 - Autobaud error
						-2 - Unable to init hardware
						-3 - CAN module not ready
 */ 
int32_t CAN_Init (uint32_t bdr); 

/**
  * @brief 	Sending data frame via CAN
  * @note	Using Extended ID format.  
  * @param	id 			- Frame's ID
			dlc 		- Frame's data length
			aData 		- pointer at data buf to send
  * @retval	HAL_OK 		- SUCCESS
			HAL_ERROR 	- No free Tx buffers
 */ 
HAL_StatusTypeDef CAN2B_SendDataFrame (uint32_t id, uint8_t data[], uint8_t len);

_Bool CAN_GetFrame (CANframe_t *pstore);

HAL_StatusTypeDef CAN_SetBroadcastFilter (uint32_t rx_fifo);

int8_t CAN_SelfTest(CAN_HandleTypeDef *hcan);

#endif // _CNCU_CAN_H
