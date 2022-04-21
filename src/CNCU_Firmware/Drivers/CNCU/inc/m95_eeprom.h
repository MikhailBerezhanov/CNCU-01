/**
  ******************************************************************************
  * @file           : M95_eeprom.h
  * @brief          : Header of Module working with CNCU external EEPROM
  ******************************************************************************
*/

#ifndef _CNCU_EEPROM_H
  #define _CNCU_EEPROM_H
/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "cncu_conf.h"

/* Private define ------------------------------------------------------------*/
// Type of external memory chip
#define _M95_256

// GPIO control pins
#define EEP_nCSS_Port		GPIOE
#define EEP_nCSS_Pin		GPIO_PIN_8
#define EEP_nHOLD_Port	GPIOA
#define EEP_nHOLD_Pin		GPIO_PIN_4
#define EEP_nWrite_Port	GPIOE
#define EEP_nWrite_Pin	GPIO_PIN_7

// Test debug values
#define TestVal	0x43

// Timeouts
#define TMOUT_WIP	10			// [ms] Timeout waiting Write In Progress ending

// nCSS control macroses
#define EEPROM_Select		HAL_GPIO_WritePin(EEP_nCSS_Port, EEP_nCSS_Pin, GPIO_PIN_RESET)
#define EEPROM_Deselect	HAL_GPIO_WritePin(EEP_nCSS_Port, EEP_nCSS_Pin, GPIO_PIN_SET)

// Instruction set
#define WREN			0x06		// Write Enable
#define WRDI			0x04		// Write Disable
#define RDSR			0x05		// Read Status Register
#define WRSR			0x01		// Write Status Register
#define READ			0x03		// Read from Memory Array
#define WRITE			0x02		// Write to Memory Array	
#define RIP				0x83		// Reads the page dedicated to identification
#define WIP				0x82		// Writes the page dedicated to identification
#define RLS				0x83		// Reads the lock status of the identification Page
#define LockID		0x82		// Locks th identification page in read-only mode

// Status Register's bit-flags
#define WIP_BIT		0x01		// Write in Progress status
#define WEL_BIT		0x02		// Write Enable Latch
  
// Memory Map
#ifdef _M95_256
  #define BEG_ADRR	0x0000	// Start address of memory
  #define END_ADDR	0x7FFF	// Last available address of memory
  #define ADDR_SIZE 32768		// In Bytes
#endif
#ifdef _M95_M02
  #define BEG_ADRR	0x00000	// Start address of memory
  #define END_ADDR	0x3FFFF	// Last available address of memory
  #define ADDR_SIZE 262144	// In Bytes
#endif

/* Exported types ------------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
/**
  * @brief 	Initialization of GPIO and SPI to work with EEPROM   
  * @param	None
  * @retval	0 - SUCCESS, 
			1 - Error in test Write-Read cycle 
			2 - Unable to initialize SPI
 */ 
cncu_err_t M95EEPROM_Init (void); 

/**
  * @brief 	Read data from M95 EEPROM   
  * @param	addr 	- memory address from where to start reading cycle
			size 	- amount of data to read in bytes
			*pStore - pointer at buf to store read data
  * @retval	0 - SUCCESS,
			2 - Timeout waiting finishing of Writing Cycle (WIP_BIT is set)
 */ 
cncu_err_t M95EEPROM_Read (uint16_t addr, uint16_t size, uint8_t *pStore);

/**
  * @brief 	Write data to M95 EEPROM   
  * @param	addr 	- memory address where to start writing cycle
			size 	- amount of data to write in bytes
			*pData - pointer at data buf to write
  * @retval	0 - SUCCESS,
			2 - Unable to set WriteEnable (WEL_BIT)
 */ 
cncu_err_t M95EEPROM_Write (uint16_t addr, uint16_t size, uint8_t *pData);

#endif // _CNCU_EEPROM_H
