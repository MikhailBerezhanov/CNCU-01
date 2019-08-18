/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
*/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H__
#define __MAIN_H__

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* Private define ------------------------------------------------------------*/
// Debug ports choosing (can be both)
#define DEBUG_PORT_USB			1
#define DEBUG_PORT_UART			1

// Modules to debug
#define DEBUG_MAIN					1	
#define DEBUG_CAN						1
#define DEBUG_LWIP					1
#define DEBUG_I2C						1
#define	DEBUG_UART					1
#define DEBUG_SPI						1
#define	DEBUG_EEPROM				1
#define	DEBUG_MK_INIT				1
#define	DEBUG_FIFO					1
#define	DEBUG_RS232_485			1
#define DEBUG_RTC						1
#define DEBUG_SD						1
#define DEBUG_FLASH					1

// Hardware tests
#define TEST_EEPROM					1

// WEB server settings
#define DHCP_ENABLE					1

// Timeouts
#define TMOUT_DHCP					5000				// [ms] to get IP with DHCP
#define PING_SERV_PERIOD		1000				// [ms] to ping remote server

// IRQ Priorities (Preemption Priority and Sub Priority)
#define SysTick_PreP  15		// System ticks for RTOS
#define	SysTick_SubP	0
#define ETH_PreP			5     // Ethernet frame reception
#define	ETH_SubP			0			
#define	USART1_PreP		5			// USB serial
#define	USART1_SubP		0
#define	USART2_PreP		5			// RS-232 or RS-485
#define	USART2_SubP		0
#define	USART3_PreP		1			// Debug UART
#define	USART3_SubP		0
#define CAN_RX0_PreP	4			// RX0 (CAN1)
#define CAN_RX0_SubP	0	
#define CAN_RX1_PreP	0			// RX1 (CAN1)
#define CAN_RX1_SubP	0
#define CAN_SCE_PreP	0			// SCE (CAN1)
#define CAN_SCE_SubP	0	
#define GTIM_PreP			0			// GeneralPurpose Timer (TIM10)
#define GTIM_SubP			0			
#define LTIM_PreP			0			// LED Timer (TIM11)
#define LTIM_SubP			0			
#define	I2C3_PreP			0			// I2C external bus
#define I2C3_SubP			0
#define SPI1_PreP			0			// SPI onboard+external bus
#define	SPI1_SubP			0

// Default settings
#define MY_DEFAULT_IP			"192.168.3.33"
#define SERV_DEFAULT_IP		"192.168.3.166"
#define SERV_DEFAULT_PORT	3333
#define	MY_CAN_ADDR       0
#define CAN_BAUD					100000

/* Exported types ------------------------------------------------------------*/
// States of ETH physical and logical connection
typedef enum
{
	UP,												// PHY connection
	DOWN											//
}lstate_t;

// States of connection with remote server
typedef enum
{
	ONLINE,									// Remote server is online
	OFFLINE								// Remote server is offline
}sstate_t;

// States of work process
typedef enum
{
	CONNECTING,								// Server connection
	CONNECTED,								//  	
	GET_SETTINGS,
	WORK
}pstate_t;

// CNCU status and configuration structure
typedef __packed struct 
{
	__packed struct
	{
		uint8_t SysClockstatus;	// System Clock
		uint8_t LEDSstatus;			// LEDS and TIM11
		uint8_t USART1status;		//
		uint8_t USART2status;		//
		uint8_t USART3status;		//
		uint8_t SPI1status;			//
		uint8_t SPI3status;			//
		uint8_t I2C3status;			//
		uint8_t CAN1status;			//
		uint8_t ETHstatus;			// Ethernet
		uint8_t GTIMstatus; 		// GeneralPurpose Timer
		uint8_t RTCstatus;
		uint8_t SDstatus;
	}Periph;
	
	__packed struct
	{
		uint32_t HCLK;					// Value of Main clock
		uint32_t PCLK1;					// Value of Peripheral clock 1
		uint32_t PCLK2;					// Value of Peripheral clock 2
		uint8_t  DSWTCH;				// Value of onboard switches
		int32_t  CAN_SPEED;			// CAN speed [b\s]
		uint32_t IP_ADDR;
		uint8_t  MAC_ADDR;
	}Config;
	
	__packed struct
	{
		uint32_t 	myIP;					// CNCU IP address
		uint32_t 	servIP;				// External server's IP address
		uint16_t	servPort;			// External server's port
		uint8_t		myMAC[6];			// CNCU MAC address
		uint8_t 	myCANaddr;		// CNCU CAN address
		uint32_t	myCANbaud;		// CNCU CAN baudrate [b\s]
	}Settings;
	
	__packed struct
	{
		lstate_t link;					// ETH connection state	
		pstate_t proc;					// Work process state
		sstate_t serv;					// Remote Server state
	}States;
}TDevStatus;

/* Exported variables --------------------------------------------------------*/
extern TDevStatus CNCU;

#endif /* __MAIN_H__ */

