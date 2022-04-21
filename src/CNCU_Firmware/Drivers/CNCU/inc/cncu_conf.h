/**
  ******************************************************************************
  * @brief          : Config of drivers for CNCU board
  ******************************************************************************
*/

#ifndef _CNCU_CONF_H
#define _CNCU_CONF_H

/* Includes ------------------------------------------------------------------*/

#define CNCU_BOARD_VERSION	2

// 
#define _TRUE								1
#define _FALSE							0

// Modules to debug
#define DEBUG_CAN						_TRUE
#define DEBUG_I2C						_TRUE
#define	DEBUG_UART					_TRUE
#define DEBUG_SPI						_TRUE
#define	DEBUG_EEPROM				_TRUE
#define	DEBUG_FIFO					_TRUE
#define	DEBUG_RS232_485			_FALSE
#define DEBUG_RTC						_TRUE
#define DEBUG_SD						_TRUE
#define DEBUG_FLASH					_TRUE
#define DEBUG_BUZZ					_TRUE
#define DEBUG_SD						_TRUE

// Hardware tests
#define EEPROM_SELFTEST			_FALSE

// IRQ Priorities (Preemption Priority and Sub Priority)
#define SysTick_PreP  			15		// System ticks for RTOS
#define	SysTick_SubP				0
#define ETH_PreP						5     // Ethernet frame reception
#define	ETH_SubP						0			
#define	USART1_PreP					5			// USB serial
#define	USART1_SubP					1
#define	USART2_PreP					5			// RS-232 or RS-485
#define	USART2_SubP					0
#define	USART3_PreP					1			// Debug UART
#define	USART3_SubP					0
#define CAN_RX0_PreP				4			// RX0 (CAN1)
#define CAN_RX0_SubP				0	
#define CAN_RX1_PreP				0			// RX1 (CAN1)
#define CAN_RX1_SubP				0
#define CAN_SCE_PreP				0			// SCE (CAN1)
#define CAN_SCE_SubP				0	
#define GTIM_PreP						0			// GeneralPurpose Timer (TIM10)
#define GTIM_SubP						0			
#define LTIM_PreP						0			// LED Timer (TIM11)
#define LTIM_SubP						0			
#define	I2C1_PreP						0			// I2C external bus
#define I2C1_SubP						0
#define SPI1_PreP						0			// SPI onboard+external bus
#define	SPI1_SubP						0
#define	BuzzTIM_PreP				1
#define BuzzTIM_SubP				0
#define SD_Global_PreP			4
#define SD_Global_SubP			0
#define SD_DMA_RX_PreP			5
#define SD_DMA_RX_SubP			0
#define SD_DMA_TX_PreP			5
#define SD_DMA_TX_SubP			0

/* Exported types ------------------------------------------------------------*/
typedef enum 
{
	CNCU_OK = 0,
	CNCU_TMOUT,
	INIT_ERR,
	WRITE_ERR,
	READ_ERR,
	DATA_ERR,
	STATUS_ERR	
}cncu_err_t;

/* Exported variables --------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/ 

#endif // _CNCU_UART_H
