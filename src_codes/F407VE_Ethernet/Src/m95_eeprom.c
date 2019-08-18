/**
  ******************************************************************************
  * @file           : m95_eeprom.c				  
  * @brief          : Module working with CNCU external EEPROM M95256 or M95M02
  * @note			: Use your implementations for M95EEPROM_Init, SPI_Send, 
					  and SPI_Get to port this driver.
  ******************************************************************************
*/
#include "m95_eeprom.h"
#include "spi.h"
#include "gtimer.h"

#define DEBUG_MSG DEBUG_EEPROM
#include "dbgmsg.h"

static HAL_StatusTypeDef EEPROM_Read_SR (uint8_t *SR_val);
static uint8_t EEPROM_Check_WIP (void);
static uint8_t EEPROM_Check_WEL (void);

volatile uint32_t eep_timer = TMOUT_WIP;

/**
	@brief Initialization of GPIO and SPI to work with EEPROM
 */ 
uint8_t M95EEPROM_Init (void)
{
  GPIO_InitTypeDef GPIO_InitStruct;
	
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
	
  // CSS and nWrite configure
  GPIO_InitStruct.Pin = EEP_nCSS_Pin | EEP_nWrite_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(EEP_nCSS_Port, &GPIO_InitStruct);	
		
  // nHOLD configure
  GPIO_InitStruct.Pin = EEP_nHOLD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(EEP_nHOLD_Port, &GPIO_InitStruct);
	
	// GPIO pin Output Level
  EEPROM_Deselect;		
  HAL_GPIO_WritePin(EEP_nWrite_Port, EEP_nWrite_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(EEP_nHOLD_Port, EEP_nHOLD_Pin, GPIO_PIN_SET);	
	
#if TEST_EEPROM
  // Testing Writing and Reading Cycles
  
  uint8_t tmp = TestVal;
 
  if(M95EEPROM_Write(END_ADDR, 1, &tmp))
  {
		errmsg("-EEPROM: Error writing to Memory Array\r\n");
		return WRITE_ERROR;
  }
 
  tmp = 0;
  
  if (!M95EEPROM_Read(END_ADDR, 1, &tmp))
  {
		dbgmsg("-EEPROM: Read_value:\t0x%02X\r\n", tmp); 	  
  }
  else 
  {
		errmsg("-EEPROM: Error reading from Memory Array\r\n");
		return READ_ERROR;
  }
	
  if (tmp == TestVal) return EEPROM_OK;
 
  return INVALID_DATA;
#else  
  return EEPROM_OK;
#endif
}

/**
	@brief Write data to M95
 */ 
uint8_t M95EEPROM_Write (uint16_t addr, uint16_t size, uint8_t *pData)
{
  uint8_t Instr = 0;						// Instruction to send
  uint8_t AddrH = (uint8_t) (addr >> 8);	// 8 High bits of addr
  uint8_t AddrL	= (uint8_t) addr;			// 8 Low  bits of addr
  uint8_t *pbuf = pData;	
		
  // SET WREN bit to enable Write operation
  if (!EEPROM_Check_WEL())
  {
		Instr = WREN;
		EEPROM_Select;
		SPI_Transmit(eSPI1, &Instr, 1);	
		EEPROM_Deselect;
  }
	
  if (EEPROM_Check_WEL() != WEL_BIT) return STATUS_ERROR; 
  
  Instr = WRITE;
  EEPROM_Select;
  SPI_Transmit(eSPI1, &Instr, 1);	
  SPI_Transmit(eSPI1, &AddrH, 1);
  SPI_Transmit(eSPI1, &AddrL, 1);
  while (size--)
  {
		SPI_Transmit(eSPI1, pbuf, 1);
		pbuf++;  
  }	
  EEPROM_Deselect;
  
  // Disable Write operation
  Instr = WRDI;
  EEPROM_Select;
  SPI_Transmit(eSPI1, &Instr, 1);	
  EEPROM_Deselect;	
  
  return EEPROM_OK;
}

/**
	@brief Read data from to M95
 */ 
uint8_t M95EEPROM_Read (uint16_t addr, uint16_t size, uint8_t *pStore)
{
  uint8_t Instr = READ;
  uint8_t AddrH = (uint8_t) (addr >> 8);	// 8 High bits of addr
  uint8_t AddrL	= (uint8_t) addr;					// 8 Low  bits of addr
  uint8_t *pbuf = pStore; 	
		
  // Checking Writing In Progress status
  if (EEPROM_Check_WIP() == WIP_BIT) 
  {
		eep_timer = TMOUT_WIP;
		while (eep_timer)
		{
			if (!EEPROM_Check_WIP()) break;
		}
	if (!eep_timer) return STATUS_ERROR;
  }
  	
  EEPROM_Select;
  SPI_Transmit(eSPI1, &Instr, 1);	
  SPI_Transmit(eSPI1, &AddrH, 1);
  SPI_Transmit(eSPI1, &AddrL, 1);	
  while (size--)
  {
		SPI_Receive(eSPI1, pbuf, 1);
		pbuf++;  
  }	
  EEPROM_Deselect;
		
  return EEPROM_OK;
}

/**
  * @brief 	Read Status Register of EEPROM.
  * @param	*SR_val - pointer at byte to store read value
  * @retval	HAL_OK - SUCCESS, HAL_ERROR -  FAILURE
 */
static HAL_StatusTypeDef EEPROM_Read_SR (uint8_t *SR_val)
{
  uint8_t Instr = RDSR;
  HAL_StatusTypeDef res;	
	
  EEPROM_Select;
  SPI_Transmit(eSPI1, &Instr, 1);
  res = SPI_Receive(eSPI1, SR_val, 1);

  if (res == HAL_OK)
	  dbgmsg("-EEPROM StatusReg_value:\t0x%02X\r\n", *SR_val);
  else errmsg("Error reading EEPROM StatusReg\r\n");

  EEPROM_Deselect;
  
  return res;
}

/**
  * @brief 	Checking if EEPROM is in Write in Progress status.
  * @param	None
  * @retval 0x01 - Write IS in progress, 
			0x00 - Write IS NOT in progress
			STATUS_ERROR - Error while reading status reg
 */
static uint8_t EEPROM_Check_WIP (void)
{
  uint8_t SR;
  
  if (EEPROM_Read_SR(&SR) == HAL_OK) return (SR & WIP_BIT);	
  else return STATUS_ERROR;
}

/**
  * @brief 	Checking if EEPROM's Write Enable Latch is set.
  * @param	None
  * @retval 0x02 - Write IS Enabled, 
			0x00 - Write IS NOT ENABLED
			STATUS_ERROR - Error while reading status reg
 */
static uint8_t EEPROM_Check_WEL (void)
{
  uint8_t SR;
  
  if (EEPROM_Read_SR(&SR) == HAL_OK) return (SR & WEL_BIT);	
  else return STATUS_ERROR;
}
