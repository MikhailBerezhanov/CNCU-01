/**
  ******************************************************************************
  * @file           : flash.c
  * @brief          : Module working with CNCU external FLASH memory AT45DB161E
	*									Page (512/528 bytes)
  ******************************************************************************
*/
#include "spi.h"
#include "gtimer.h"
#include "flash.h"

#define DEBUG_MSG DEBUG_FLASH
#include "dbgmsg.h"

volatile uint32_t fl_timer = WAITRDY_TMOUT;
struct{
	pagesize_t pagesize;
}FL_info;

// Read AT45DB161E Status register (2 bytes) 
static cncu_err_t FLASH_Read_SR(uint8_t *SR_val)
{
	uint8_t opcode = STATUS_REG;
  HAL_StatusTypeDef res;	
	
  FLASH_Select;
  SPI_Transmit(&opcode, 1);
	
  if ((res = SPI_Receive((uint8_t*)&SR_val[0], 1)) != HAL_OK)
	{		
		errmsg("Error reading FLASH StatusReg1: %d\r\n", res);
		return STATUS_ERR;
	}
	//dbgmsg("SR byte1: 0x%02X\r\n", (uint8_t)SR_val[0]);
	
  if ((res = SPI_Receive((uint8_t*)&SR_val[1], 1)) != HAL_OK) 
	{
		errmsg("Error reading FLASH StatusReg2: %d\r\n", res);
		return STATUS_ERR;
	}
	//dbgmsg("SR byte2: 0x%02X\r\n", (uint8_t)SR_val[1]);

  FLASH_Deselect;
  
  return CNCU_OK;
}

// Check if FLASH is ready (RDY\BUSY)
static _Bool FLASH_IsReady(void)
{
	uint8_t tmp[2];
	
	if (FLASH_Read_SR(tmp) == CNCU_OK)
	{
		if (0x80 & tmp[0]) return 1;
	}
	
	return 0;
}

// Check Erase\Program Error (EPE)
static _Bool FLASH_IsEPE(void)
{
	uint8_t tmp[2];
	
	if (FLASH_Read_SR(tmp) == CNCU_OK)
	{
		if (!(0x20 & tmp[1])) return 0;
	}
	
	return 1;
}

// Get FLASH Page Size Configuration 
static pagesize_t FLASH_GetPageSize(void)
{
	uint8_t tmp[2];
	
	if (FLASH_Read_SR(tmp) == CNCU_OK)
	{
		if (!(0x01 & tmp[0])) return ps528;
		else return ps512;
	}
	
	return psUnkwn;
}

// Wait for Ready state after Write or Erase command 
static cncu_err_t FLASH_WaitForReady(void)
{	
	uint32_t tickstart = HAL_GetTick();
		
	while(!FLASH_IsReady())
	{
		if ((HAL_GetTick()-tickstart) >= WAITRDY_TMOUT) return CNCU_TMOUT;
	}
	
	return CNCU_OK;
}

// Buffer and Page Size Configuration
static cncu_err_t FLASH_ChangePageSize(pagesize_t psize)
{
	uint8_t opcode[4] = {0x3D, 0x2A, 0x80, 0xA6};
	HAL_StatusTypeDef res;	
	
	switch (psize)
	{
		case ps512: 
			break;
		
		case ps528:
			opcode[3]= 0xA7;
			break;
		
		default: return DATA_ERR;
	}
	
	FLASH_Select;
	res = SPI_Transmit(opcode, 4);
	FLASH_Deselect;
	
	if (res == HAL_OK)
	{
		if (FLASH_WaitForReady() == CNCU_OK)
		{
			if (FLASH_GetPageSize() == psize) return CNCU_OK;
		}
	}
	return WRITE_ERR;
}

//
cncu_err_t FLASH_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	
	__HAL_RCC_GPIOB_CLK_ENABLE();
	
	// CSS configure
  GPIO_InitStruct.Pin = FL_nCSS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(FL_nCSS_Port, &GPIO_InitStruct);
	
	FLASH_Deselect;
	
	uint8_t tmp[5];
	uint8_t len = 0;
	
	if (FLASH_GetID(tmp, &len) == CNCU_OK)
	{
		print_arr("-FLASH ID: ", tmp, len);
		
		FL_info.pagesize = FLASH_GetPageSize();
		if (FL_info.pagesize != psUnkwn) dbgmsg("-FLASH PageSize: %d bytes\r\n", FL_info.pagesize);
		else 
		{
			errmsg("-FLASH Invalid PageSize\r\n");
			return DATA_ERR;
		}
		
		if (FL_info.pagesize != FL_PAGE_SIZE)
		{
			dbgmsg("-FLASH Reconfiguring PageSize from %d to %d...\r\n", FL_info.pagesize, FL_PAGE_SIZE);
			if (FLASH_ChangePageSize(FL_PAGE_SIZE) != CNCU_OK)
			{
				errmsg("-FLASH PageSize reconfiguration failed\r\n");
				return STATUS_ERR;
			}
			FL_info.pagesize = FLASH_GetPageSize();
			if (FL_info.pagesize != psUnkwn) dbgmsg("-FLASH New PageSize: %d bytes\r\n", FL_info.pagesize);
		}
		
		if ((tmp[1] == 0x26) && (tmp[2] == 0x00)) return CNCU_OK;
	}
	
	return INIT_ERR;
}

// Manufacturer and Device ID Read
cncu_err_t FLASH_GetID(uint8_t *id, uint8_t *idlen)
{
	uint8_t Instr = READ_DEV_ID;
  HAL_StatusTypeDef res;	
	
  FLASH_Select;
  SPI_Transmit(&Instr, 1);
	
	for(int i = 0; i < DEV_ID_LEN; i++)
	{
		if ((res = SPI_Receive(&id[i], 1)) != HAL_OK)
		{		
			errmsg("Error reading FLASH ID[%d]: %d\r\n", i, res);
			return STATUS_ERR;
		}
	}
	
  FLASH_Deselect;
	
  *idlen = DEV_ID_LEN;
	
  return CNCU_OK;	
}

// Create 3-byte memory address from Page number and Byte number
static void FLASH_MakeMemAddr(uint16_t pagenum, uint16_t bytenum, uint8_t* addr)
{
	memset(addr, 0, 3);

	// PA = 12bit, BA = 10(9)bit
	if (FL_info.pagesize == ps528)
	{
		addr[0] = (uint8_t)(pagenum >> 4) >> 2; // 2 dummy bits
		addr[1] = ((uint8_t)pagenum << 2) | ((uint8_t)(bytenum >> 8));
	}
	else 
	{
		addr[0] = (uint8_t)(pagenum >> 4) >> 3; // 3 dummy bits
		addr[1] = ((uint8_t)pagenum << 1) | ((uint8_t)(bytenum >> 8));
	}
	addr[2] = (uint8_t)bytenum;	
	
	print_arr("FLASH_MakeMemAddr: ", addr, 3);
}

// TODO: Set higher SPI Freq before reading, and then restore
// Continuous Array Read. 
// NOTE: This version uses [High Frequency Mode 85 MHz MAX] opcode
cncu_err_t FLASH_ContRead(uint16_t pagenum, uint16_t bytenum, uint8_t* buf, uint32_t len)
{
	if(FLASH_WaitForReady() != CNCU_OK) return CNCU_TMOUT;
	
	if ((pagenum >= FL_PAGE_NUM) || (bytenum >= FL_PAGE_SIZE))
	{
		errmsg("Invalid memory address\r\n");
		return DATA_ERR;
	}
	
	uint8_t addr[3];
	FLASH_MakeMemAddr(pagenum, bytenum, addr);
	
  uint8_t opcode[8] = {ARR_READ_MIDF};	// +4 dummy bytes
	memcpy(&opcode[1], addr, 3);
	
	FLASH_Select;
	
	if(SPI_Transmit(opcode, 8) == HAL_OK)
	{
		if (SPI_Receive(buf, len) != HAL_OK) 
		{
			errmsg("Error reading FLASH page:\r\n");
			return READ_ERR;
		}	
	}
	
	FLASH_Deselect;	
	
	return CNCU_OK;
}

// Unlike the Continuous Array Read, when the end of a page in main memory 
// is reached, the device will continue reading back at the beginning of the 
// same page rather than the beginning of the next page.
cncu_err_t FLASH_ReadPage(uint16_t pagenum, uint16_t bytenum, uint8_t* buf, uint16_t len)
{
	if(FLASH_WaitForReady() != CNCU_OK) return CNCU_TMOUT;
	
	if ((pagenum >= FL_PAGE_NUM) || (bytenum >= FL_PAGE_SIZE))
	{
		errmsg("Invalid memory address\r\n");
		return DATA_ERR;
	}
	
	uint8_t addr[3];
	FLASH_MakeMemAddr(pagenum, bytenum, addr);
		
  uint8_t opcode[8] = {PAGE_READ};	// +4 dummy bytes
	memcpy(&opcode[1], addr, 3);
	
	FLASH_Select;
	
	if(SPI_Transmit(opcode, 8) == HAL_OK)
	{
		if (SPI_Receive(buf, len) != HAL_OK) 
		{
			errmsg("Error reading FLASH page:\r\n");
			return READ_ERR;
		}	
	}
	
	FLASH_Deselect;	
	
	return CNCU_OK;
}

// TODO: Rework this function into FLASH_WritePage.
// TODO: Set higher SPI Freq before operation, and then restore
// NOTE: Trash from the internal device's bufer also programmed into memory page
// In that case FULL page is programmed. To write only user's bytes use
// FLASH_Erase, FLASH_Write functions
cncu_err_t FLASH_EraseAndWrite(uint16_t pagenum, uint16_t bytenum, uint8_t* buf, uint32_t len)
{
	if(FLASH_WaitForReady() != CNCU_OK) return CNCU_TMOUT;
	
	if ((pagenum >= FL_PAGE_NUM) || (bytenum >= FL_PAGE_SIZE))
	{
		errmsg("Invalid memory address\r\n");
		return DATA_ERR;
	}
		
	uint8_t addr[3];
	FLASH_MakeMemAddr(pagenum, bytenum, addr);
	
	uint8_t opcode[4] = {PAGE_ER_WRITE1,0,0,0};
	memcpy(&opcode[1], addr, 3);
	
	FLASH_Select;
	
	if(SPI_Transmit(opcode, 4) == HAL_OK)
	{
		if (SPI_Transmit(buf, len) != HAL_OK) 
		{
			errmsg("Error writing FLASH page\r\n");
			return WRITE_ERR;
		}	
	}
	
	FLASH_Deselect;
	
	if (FLASH_IsEPE())
	{
		errmsg("Erase or Program error arised\r\n");
		return WRITE_ERR;
	}
	
	return CNCU_OK;
}

//
// NOTE: only the bytes clocked into the buffer are programmed into the pre-erased 
// byte locations in main memory.  Multiple bytes up to the pagesize can be entered
cncu_err_t FLASH_WriteBytes(uint16_t pagenum, uint16_t bytenum, uint8_t* buf, uint32_t len)
{
	if(FLASH_WaitForReady() != CNCU_OK) return CNCU_TMOUT;
	
	if ((pagenum >= FL_PAGE_NUM) || (bytenum >= FL_PAGE_SIZE))
	{
		errmsg("Invalid memory address\r\n");
		return DATA_ERR;
	}
		
	uint8_t addr[3];
	FLASH_MakeMemAddr(pagenum, bytenum, addr);
	
	uint8_t opcode[4] = {PAGE_BYTE_WRITE,0,0,0};
	memcpy(&opcode[1], addr, 3);
	
	FLASH_Select;
	
	if(SPI_Transmit(opcode, 4) == HAL_OK)
	{
		if (SPI_Transmit(buf, len) != HAL_OK) 
		{
			errmsg("Error writing FLASH page\r\n");
			return WRITE_ERR;
		}	
	}
	
	FLASH_Deselect;
	
	if (FLASH_IsEPE())
	{
		errmsg("Erase or Program error arised\r\n");
		return WRITE_ERR;
	}
	
	return CNCU_OK;
}

// Reprogram any number of sequential bytes in a page in the main memory array 
// WITHOUT affecting the rest of the bytes in the same page without pre-erasing
cncu_err_t FLASH_ReadModifyWrite(uint16_t pagenum, uint16_t bytenum, uint8_t* buf, uint32_t len)
{
	if(FLASH_WaitForReady() != CNCU_OK) return CNCU_TMOUT;
	
	if ((pagenum >= FL_PAGE_NUM) || (bytenum >= FL_PAGE_SIZE))
	{
		errmsg("Invalid memory address\r\n");
		return DATA_ERR;
	}
		
	uint8_t addr[3];
	FLASH_MakeMemAddr(pagenum, bytenum, addr);
	
	uint8_t opcode[4] = {READ_MOD_WRITE1,0,0,0};
	memcpy(&opcode[1], addr, 3);
	
	FLASH_Select;
	
	if(SPI_Transmit(opcode, 4) == HAL_OK)
	{
		if (SPI_Transmit(buf, len) != HAL_OK) 
		{
			errmsg("Error writing FLASH page\r\n");
			return WRITE_ERR;
		}	
	}
	
	FLASH_Deselect;
	
	if (FLASH_IsEPE())
	{
		errmsg("Erase or Program error arised\r\n");
		return WRITE_ERR;
	}
	
	return CNCU_OK;
}

// Memory Page Erase (0xFF)
cncu_err_t FLASH_ErasePage(uint16_t pagenum)
{
	if (FLASH_WaitForReady() != CNCU_OK) return CNCU_TMOUT;
	
	if (pagenum >= FL_PAGE_NUM)
	{
		errmsg("Invalid memory address\r\n");
		return DATA_ERR;
	}
	
	uint8_t addr[3];
	FLASH_MakeMemAddr(pagenum, 0, addr);
	
	uint8_t opcode[4] = {PAGE_ERASE,0,0,0};
	memcpy(&opcode[1], addr, 3);
	
	FLASH_Select;
	if(SPI_Transmit(opcode, 4) != HAL_OK)
	{
		errmsg("SPI_Transmit failed\r\n");
		return WRITE_ERR;
	}
	FLASH_Deselect;
	
	if (FLASH_IsEPE())
	{
		errmsg("Erase or Program error arised\r\n");
		return WRITE_ERR;
	}
	
	return CNCU_OK;
}

// Memory Block Erase (8 Pages) 
cncu_err_t FLASH_EraseBlock(uint16_t blocknum)
{
	if (FLASH_WaitForReady() != CNCU_OK) return CNCU_TMOUT;
	
	if (blocknum >= FL_BLOCK_NUM)
	{
		errmsg("Invalid memory address\r\n");
		return DATA_ERR;
	}
	
	uint8_t addr[3];
	// Block addr is 9 bit lenght
	if (FL_info.pagesize == ps528)
	{
		addr[0] = (uint8_t)(blocknum >> 3) & 0x3F; // 2 dummy bits
		addr[1] = ((uint8_t)blocknum << 5) & 0xE0;
	}
	else 
	{
		addr[0] = (uint8_t)(blocknum >> 4) & 0x1F; // 3 dummy bits
		addr[1] = ((uint8_t)blocknum << 4) & 0xF0;
	}
	addr[2] = 0;	
	
	print_arr("Block Erase addr: ", addr, 3);
	
	uint8_t opcode[4] = {PAGE_ERASE,0,0,0};
	memcpy(&opcode[1], addr, 3);
	
	FLASH_Select;
	if(SPI_Transmit(opcode, 4) != HAL_OK)
	{
		errmsg("SPI_Transmit failed\r\n");
		return WRITE_ERR;
	}
	FLASH_Deselect;
	
	if (FLASH_IsEPE())
	{
		errmsg("Erase or Program error arised\r\n");
		return WRITE_ERR;
	}
	
	return CNCU_OK;
}

// Memory Sector Erase (256 Pages)
// NOTE: It may take up to 2 sec to perform sector erase (to restore Ready state)
cncu_err_t FLASH_EraseSector(uint16_t sectornum)
{
	return CNCU_OK;
}

// Full Chip Erase
// NOTE: It may take up to 40 sec to perform full erase (to restore Ready state)
cncu_err_t FLASH_EraseChip(void)
{
	if (FLASH_WaitForReady() != CNCU_OK) return CNCU_TMOUT;
	
	uint8_t opcode[4] = {0xC7, 0x94, 0x80, 0x9A};
	
	FLASH_Select;
	if(SPI_Transmit(opcode, 4) != HAL_OK)
	{
		errmsg("SPI_Transmit failed\r\n");
		return WRITE_ERR;
	}
	FLASH_Deselect;
	
	if (FLASH_IsEPE())
	{
		errmsg("Erase or Program error arised\r\n");
		return WRITE_ERR;
	}
	
	return CNCU_OK;
}





