/**
  ******************************************************************************
  * @file           : flash.h
  * @brief          : Header of Module working with CNCU external FLASH memory
  ******************************************************************************
*/

#ifndef _CNCU_FLASH_H
  #define _CNCU_FLASH_H
/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "cncu_conf.h"

/* Private define ------------------------------------------------------------*/
// GPIO control pins
#define FL_nCSS_Port		GPIOB
#define FL_nCSS_Pin			GPIO_PIN_0

// nCSS control macroses
#define FLASH_Select		HAL_GPIO_WritePin(FL_nCSS_Port, FL_nCSS_Pin, GPIO_PIN_RESET)
#define FLASH_Deselect	HAL_GPIO_WritePin(FL_nCSS_Port, FL_nCSS_Pin, GPIO_PIN_SET)

#define USE_512_PAGESIZE	_FALSE		// NOTE: Default pagesize = 528 [bytes]. 

// Timeouts
#define WAITRDY_TMOUT		100		// [ms]

// Memory dimensions
#define FL_PAGE_NUM			4096
#define FL_BLOCK_NUM		512
#define FL_SECTOR_NUM		16

#if USE_512_PAGESIZE 
	#define FL_PAGE_SIZE	512		// [Bytes] . Can be configured as 512 or 528
#else
	#define FL_PAGE_SIZE	528
#endif
#define FL_BLOCK_SIZE		8			// [Pages]
#define FL_SECTOR_SIZE  256		// [Pages] . Sector 0a = 8 pages, 0b = 248 pages

// Commands opcodes
#define ARR_READ_LOWF		0x03	// Continuous Array Read (Low Frequency Mode 50 MHz)
#define	ARR_READ_HIGHF	0x1B	// Continuous Array Read (High Frequency Mode 104 MHz)
#define	ARR_READ_MIDF		0x0B	// Continuous Array Read (High Frequency Mode 85 MHz)
#define	ARR_READ_LOWP		0x01	// Continuous Array Read (Low Power Mode 15 MHz max)
#define	PAGE_READ				0xD2	// Main Memory Page Read
#define BUF1_READ				0xD4	// SRAM Buffer1 Read   	(0xD1 for lower Freq)
#define BUF2_READ				0xD6	// SRAM Buffer2 Read		(0xD3 for lower Freq)
#define BUF1_WRITE			0x84	// SRAM Buffer1 Write command
#define BUF2_WRITE			0x87  // SRAM Buffer2 Write command
#define	BUF1_TO_PAGE		0x83	// SRAM Buffer1 to Main Memory Page Program with Built-In Erase command
#define	BUF2_TO_PAGE		0x86	// SRAM Buffer1 to Main Memory Page Program with Built-In Erase command
// The Main Memory Page Program through Buffer with Built-In Erase command combines the 
// Buffer Write and Buffer to Main Memory Page Program with Built-In Erase
#define PAGE_ER_WRITE1	0x82	// Combines BUF1_WRITE and BUF1_TO_PAGE 
#define PAGE_ER_WRITE2	0x85	// Combines BUF2_WRITE and BUF2_TO_PAGE
// The Main Memory Byte/Page Program through Buffer 1 without Built-In Erase command combines both the Buffer Write
// and Buffer to Main Memory Program without Built-In Erase operations to allow any number of bytes (1 to 512/528 bytes)
// to be programmed directly into previously erased locations in the main memory array
#define PAGE_BYTE_WRITE	0x02	// No Page-Erase done, only writing
#define	READ_MOD_WRITE1	0x58	// Read-modify-write operation using Buffer1	
#define	READ_MOD_WRITE2	0x59	// Read-modify-write operation using Buffer2	
#define PAGE_ERASE			0x81
#define BLOCK_ERASE			0x50
#define SECTOR_ERASE		0x7C
#define	CHIP_ERASE			0xC794809A
#define	EN_SECTOR_PROT	0x3D2A7FA9	// Enable Sector Protection
#define DIS_SECTOR_PROT	0x3D2A7F9A	// Disable Sector Protection

#define SECTOR_PROT_REG	0x32	// Read Sector Protection Register
#define SECTOR_LOCK_REG	0x35	// Read Sector Lockdown Register
#define WR_SECURITY_REG	0x9B000000	// Programming the Security Register
#define RD_SECURITY_REG	0x77	// Reading the Security Register
#define STATUS_REG			0xD7	// Status Register Read

#define PAGESIZE_TO_512	0x3D2A80A6	// Buffer and Page Size Configuration
#define PAGESIZE_TO_528	0x3D2A80A7	// Buffer and Page Size Configuration
#define READ_DEV_ID			0x9F	// Manufacturer and Device ID Read
#define DEV_ID_LEN			5			// Manufacturer and Device ID lengh [Bytes]
#define SOFT_RESET			0xF0000000	// Software Reset 

/* Exported types ------------------------------------------------------------*/ 
// Supported device's page sizes formats
typedef enum
{
	psUnkwn = 0,
	ps512 = 512,
	ps528 = 528
}pagesize_t;

/* Exported functions --------------------------------------------------------*/

cncu_err_t FLASH_Init(void);

cncu_err_t FLASH_GetID(uint8_t *id, uint8_t *idlen);

cncu_err_t FLASH_ErasePage(uint16_t pagenum);

cncu_err_t FLASH_WriteBytes(uint16_t pagenum, uint16_t bytenum, uint8_t* buf, uint32_t len);

cncu_err_t FLASH_EraseAndWrite(uint16_t pagenum, uint16_t bytenum, uint8_t* buf, uint32_t len);

cncu_err_t FLASH_ReadModifyWrite(uint16_t pagenum, uint16_t bytenum, uint8_t* buf, uint32_t len);

// bytenum - starting byte address
cncu_err_t FLASH_ReadPage(uint16_t pagenum, uint16_t bytenum, uint8_t* buf, uint16_t len);

cncu_err_t FLASH_ContRead(uint16_t pagenum, uint16_t bytenum, uint8_t* buf, uint32_t len);

cncu_err_t FLASH_EraseChip(void);

#endif




