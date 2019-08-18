/*
Source file for SD CARD operations, stm32f407vet
Based on HAL
*/

#include "sdcard.h"
#include "ctype.h"	//toupper

#define DEBUG_MSG DEBUG_SD
#include "dbgmsg.h"

/*INITIALIZATION*/
SD_HandleTypeDef hsd; //sd card structure object

OFLIST sd_opnd_files[_FS_LOCK];		//list of opened files
FILINFO sd_file_info[_FS_LOCK];	//struct array to store file-specific info (number of files determined by _FS_LOCK in ffconf.h)
 uint8_t f_cnt=0;			//opened files counter 
 
/**
* @brief SD MSP Initialization
* This function configures the hardware resources used in this example
* @param hsd: SD handle pointer
* @retval None
*/
void HAL_SD_MspInit(SD_HandleTypeDef* hsd)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(hsd->Instance==SDIO)
  {
    /* Peripheral clock enable */
    __HAL_RCC_SDIO_CLK_ENABLE();
  
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**SDIO GPIO Configuration    
    PC8     ------> SDIO_D0
    PC9     ------> SDIO_D1
    PC10     ------> SDIO_D2
    PC11     ------> SDIO_D3
    PC12     ------> SDIO_CK
    PD2     ------> SDIO_CMD 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11 
                          |GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDIO;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDIO;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
  }
}

/**
* @brief SD MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param hsd: SD handle pointer
* @retval None
*/
void HAL_SD_MspDeInit(SD_HandleTypeDef* hsd)
{
  if(hsd->Instance==SDIO)
  {
    /* Peripheral clock disable */
    __HAL_RCC_SDIO_CLK_DISABLE();
  
    /**SDIO GPIO Configuration    
    PC8     ------> SDIO_D0
    PC9     ------> SDIO_D1
    PC10     ------> SDIO_D2
    PC11     ------> SDIO_D3
    PC12     ------> SDIO_CK
    PD2     ------> SDIO_CMD 
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11 
                          |GPIO_PIN_12);

    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2);
  }
}
 
//Modified function HAL_SD_INIT from stm32f4xx_hal_sd.c
static HAL_StatusTypeDef SD_Init0(SD_HandleTypeDef *hsd)
{
  /* Check the SD handle allocation */
  if(hsd == NULL)
  {
    return HAL_ERROR;
  }

  /* Check the parameters */
  assert_param(IS_SDIO_ALL_INSTANCE(hsd->Instance));
  assert_param(IS_SDIO_CLOCK_EDGE(hsd->Init.ClockEdge));
  assert_param(IS_SDIO_CLOCK_BYPASS(hsd->Init.ClockBypass));
  assert_param(IS_SDIO_CLOCK_POWER_SAVE(hsd->Init.ClockPowerSave));
  assert_param(IS_SDIO_BUS_WIDE(hsd->Init.BusWide));
  assert_param(IS_SDIO_HARDWARE_FLOW_CONTROL(hsd->Init.HardwareFlowControl));
  assert_param(IS_SDIO_CLKDIV(hsd->Init.ClockDiv));

  if(hsd->State == HAL_SD_STATE_RESET)
  {
    /* Allocate lock resource and initialize it */
    hsd->Lock = HAL_UNLOCKED;
    /* Init GPIOs for SDIO interface */
    HAL_SD_MspInit(hsd);
  }

  hsd->State = HAL_SD_STATE_BUSY;

  /* Initialize the Card parameters */
  HAL_SD_InitCard(hsd);
	
	/*
	My code
	*/
	if(hsd->ErrorCode != HAL_SD_ERROR_NONE)	//if there was something wrong with init
	{
		return HAL_ERROR;											//return error (hsd->ErrorCode is still reachable to determine what's wrong)
	}
  /* Initialize the error code */
 // hsd->ErrorCode = HAL_SD_ERROR_NONE;		//already done in HAL_SD_InitCard
  
  /* Initialize the SD operation */
  hsd->Context = SD_CONTEXT_NONE;
                                                                                     
  /* Initialize the SD state */
 // hsd->State = HAL_SD_STATE_READY;			//already done in HAL_SD_InitCard

  return HAL_OK;
}

//Initialize SD/MMC card and FAT system
/*
Error log: hsd.ErrorCode
*/
uint32_t SD_CARD_Init(void)
{
	uint32_t res;
	if(hsd.State == HAL_SD_STATE_READY)	{return 0;}//if SD card initialized already
	
		hsd.Instance = SDIO;
		hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
		hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
		hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
		/*
		The default bus width after power-up or GO_IDLE_STATE
		(CMD0) is 1 bit. SET_BUS_WIDTH (ACMD6) is only valid in a transfer state, which means
		that the bus width can be changed only after a card is selected by
		SELECT/DESELECT_CARD (CMD7).
		*/
		
		hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
		hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
		hsd.Init.ClockDiv = 0;
		if (SD_Init0(&hsd) != HAL_OK)		//Init SD card with those settings (and configure pins in HAL_SD_MspInit)
		{																		//read hsd.ErrorCode for details  (see HAL_SD_InitCard and SD_PowerON functions)
			errmsg("SD_Init0() Failed\r\n");
			HAL_SD_DeInit(&hsd);						//deInit SDIO pins
			res = HAL_SD_ERROR_CC_ERR;			//internal error
			return res;
		}

		if (HAL_SD_ConfigWideBusOperation(&hsd, SDIO_BUS_WIDE_4B) != HAL_OK)
		{
			errmsg("HAL_SD_ConfigWideBusOperation() Failed\r\n");
			HAL_SD_DeInit(&hsd);						//deInit SDIO pins
			res = HAL_SD_ERROR_CC_ERR;			//internal error
			return res;
		}

		/*If everything's OK, Init FAT system (fatfs.c)*/
		MX_FATFS_Init();
		
		if(retSD)			//if SD Driver isn't linked (retSD!=0)
		{
			errmsg("MX_FATFS_Init() Failed\r\n");
			res = HAL_SD_ERROR_GENERAL_UNKNOWN_ERR;
			hsd.State = HAL_SD_STATE_ERROR;
			return res;
		}
		
		/*Mount the SD disk */
		if(f_mount(&SDFatFS,SDPath, SD_MOUNT_NOW)!=FR_OK)		//mount  disk
		{
			errmsg("Disk %s mount Failed\r\n",SDPath);
			res = HAL_SD_ERROR_GENERAL_UNKNOWN_ERR;
			hsd.State = HAL_SD_STATE_ERROR;
			return res;
		}
		
		return 0;
}

/*SD and FAT BASIC FUNCTIONS*/
/*
Get SD-card specs (required buffer size ~ 100 - 120 bytes)
*/
void sd_print_info(void)
{
	char info_str[120];
	
	if(hsd.State != HAL_SD_STATE_READY) 	//if card not ready
	{
		errmsg("SDCARD not ready\r\n");
		return;
	}
	
	switch(hsd.SdCard.CardType)				
	{
		case CARD_SDSC: sprintf(info_str,"\tSD CARD: SDSC\r\n"); break;
	
		case CARD_SDHC_SDXC: sprintf(info_str,"\tSD CARD: SDHC/XC\r\n"); break;
	
		case CARD_SECURED: sprintf(info_str,"\t!SD CARD TYPE SECURED!\r\n"); break;
		
		default: sprintf((char*)info_str,"\t!UNKNOWN TYPE!\r\n");	return;
	}					
	
	switch(hsd.SdCard.CardVersion)
	{
		case CARD_V1_X: strcat(info_str,"\tVERSION: V1_X\r\n"); break;
		
		case CARD_V2_X: strcat(info_str,"\tVERSION: V2_X\r\n"); break;
	}
	
	switch(SDFatFS.fs_type)
	{
		case FS_FAT12: strcat(info_str,"\tFS:\t FAT12\r\n"); break;
		case FS_FAT16: strcat(info_str,"\tFS:\t FAT16\r\n"); break;
		case FS_FAT32: strcat(info_str,"\tFS:\t FAT32\r\n"); break;
		case FS_EXFAT: strcat(info_str,"\tFS:\t !EXFAT!\r\n"); break;
		default: strcat(info_str,"\tFS:\t UNKNOWN!\r\n");
	}	
	
	char str[64];
	sprintf(str,"\tBLOCKS: %d\r\n\tBLOCK SZ: %d\r\n\tCAPACITY: %d KB\r\n",	\
	hsd.SdCard.BlockNbr,\
	hsd.SdCard.BlockSize,\
	(hsd.SdCard.BlockSize*hsd.SdCard.BlockNbr)/1000000);	//end sprintf
	strcat (info_str, str);
	
	dbgmsg("%s", info_str);
}

//Scan ROOT dir for files and folders
//return total number of files and folders on the disk
//Input: Diskio_drvTypeDef *SD_Drv - structure object for SD-specific functions(SD_initialize,SD_status,SD_read, etc)
//			 FATFS *FAT_stuct - struct object for FAT (FAT type, size, cluster size,etc)
//Return: uint32_t number of files/folders on disk
uint32_t sd_n_files(const Diskio_drvTypeDef *SD_Drv, FATFS *FAT_struct)
{
	uint8_t sector_buf[_MAX_SS];					//temporary buffer for sd blocks
				
				//SD_Driver.disk_read(0,(uint8_t*)sector_buf,0,1);			//read 1 sector (MBR) from address 0
				uint32_t sect_adr =0;		//current sector of FAT. Increment each time the end of sector reached
				uint8_t f_cnt=0;		//file/folder counter 
				uint16_t i;
				uint32_t clust = 0;
				_Bool end = 0;			//'no more files in FAT' flag
	/*FAT16  */
	if(SDFatFS.fs_type == FS_FAT16)
	{
		uint16_t fat16_bufsz = _MAX_SS/2; 
		uint16_t fat16_sbuf[fat16_bufsz];
		while(!end)
			{
				if(SD_Drv->disk_read(FAT_struct->drv,sector_buf,(FAT_struct->fatbase)+sect_adr*_MAX_SS,1)!=RES_OK) 		//read 1 sector of FAT
						{
						SDFile.err = FR_DISK_ERR;										//save error code to the struct
						errmsg("disk_read Failed\r\n"); 		//call error
						return 0;
						}		
				for(uint16_t idx = 0; idx < fat16_bufsz; idx++)		//transform 8-bit entries into 32-bit
						{
							fat16_sbuf[idx] = sector_buf[idx*4];
							fat16_sbuf[idx] |= sector_buf[idx*4+1]<<8;
						}
				// 0x0FFFFFF8		
				if(!sect_adr) {i=2;} else{i=0;}		//skip the first entry of FAT32 (0xFFFFFFF8)
				for( ;i<fat16_bufsz;i++)
				{
					if(clust> FAT_struct->last_clst) return f_cnt;		//if last occupied cluster reached
					if((fat16_sbuf[i])>=0xFFF8) {f_cnt++;}
					else												//if i>511
						{
							if(sect_adr>=FAT_struct->fsize-1)  {end = 1;break;}		//if it's the last sector of FAT, then no more files, exit
						}
						clust++;
				}
				sect_adr++;									//go to the next sector
			}//end while(!end)
			return f_cnt;
		}//end if FAT16
	/*FAT32 */
	else if (SDFatFS.fs_type == FS_FAT32)
	{
		uint16_t fat32_bufsz = _MAX_SS/4; 
		uint32_t fat32_sbuf[fat32_bufsz];
		while(!end)
			{
				if(SD_Drv->disk_read(FAT_struct->drv,sector_buf,(FAT_struct->fatbase)+sect_adr*_MAX_SS,1)!=RES_OK) 		//read 1 sector of FAT
						{
						SDFile.err = FR_DISK_ERR;										//save error code to the struct
						errmsg("disk_read Failed\r\n"); 		//call error
						return 0;
						}		
				for(uint16_t idx = 0; idx < fat32_bufsz; idx++)		//transform 8-bit entries into 32-bit
						{
							fat32_sbuf[idx] = sector_buf[idx*4];
							fat32_sbuf[idx] |= sector_buf[idx*4+1]<<8;
							fat32_sbuf[idx] |= sector_buf[idx*4+2]<<16;
							fat32_sbuf[idx] |= sector_buf[idx*4+3]<<24;
						}
				// 0x0FFFFFF8		
				if(!sect_adr) {i=2;} else{i=0;}		//skip the first entry of FAT32 (0xFFFFFFF8)
				for( ;i<fat32_bufsz;i++)
				{
					if(clust> FAT_struct->last_clst) return f_cnt;	//if last occupied cluster reached
					if((fat32_sbuf[i] & 0x0FFFFFFF)>=0x0FFFFFF8) {f_cnt++;}
					else												//if i>511
						{
							if(sect_adr>=FAT_struct->fsize-1)  {end = 1;break;}		//if it's the last sector of FAT, then no more files, exit
						}
						clust++;
				}
				sect_adr++;									//go to the next sector
			}//end while(!end)
			return f_cnt;
	}
	return f_cnt;
}

/*
return list of files in folder into the buffer
each file takes about 20 bytes. So if there's lots of files, use bigger buffer;
SHORT NAMES ONLY
*/
FRESULT sd_dir_files_list (char *dir_path, uint8_t *data_buf, uint32_t buf_size)
{
	//const Diskio_drvTypeDef *SD_Drv = &SD_Driver;
	//FATFS *FAT_struct = &SDFatFS;
	FRESULT res = FR_INVALID_PARAMETER;		//invalid parameter = buffer is too small
	
	if(buf_size < SDFatFS.last_clst * 20) 			//quite a dumb check, but who cares if it works
	{
			SDFile.err = res;										//save error code to the struct
		errmsg("SD RX OVERRUN\r\n"); 		//call error
		return res;
	}
	uint8_t dir_buf[_MAX_SS];					//temporary buffer for sd blocks
	uint32_t sect_adr =0;		//current sector of folder
	uint8_t etrs_per_sect = _MAX_SS/32;		//sector size / entry size = entries per sector
	uint32_t root_entries = SDFatFS.last_clst * SDFatFS.csize;	//max entries (number of occupied clusters * cluster size = occupied sectors)
	uint32_t root_sectx = root_entries*32/_MAX_SS;		//sectors occupied by root (32 - size of an entry)
	
	DIR fold;		//struct for folder
	_Bool notroot = 0;				//'not root' flag
	//typedef
	struct dir_entry_str//short names only entry struct
	{
		char f_name[8];
		char f_ext[3];
		char attr;
		char rsrv;
		char cr_time_ms;
		char cr_time_hms[2];
		char cr_date[2];
		char l_access[2];
		char EA_idx[2];
		char l_mod_t[2];
		char l_mod_d[2];
		char fst_clu[2];
		char f_size[4];                        
	}__attribute((packed)) dir_entry;
	
//	dir_entry_str dir_entry;	//struct object
	
	uint32_t f_sect;
if(dir_path) 							//if not root
{	
	res = f_opendir(&fold,dir_path);		//open directory (to obtain sector address)
	if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
		errmsg("f_opendir FAILED\r\n"); 		//call error
		return res;
		}	
		notroot = 1;	//mark that it's a folder	
f_sect	= fold.sect;		//obtain sector address of the folder
}
else 							//if root folder
	{
	if(SDFatFS.fs_type == FS_FAT16) f_sect = SDFatFS.dirbase;
	else if(SDFatFS.fs_type == FS_FAT32) f_sect = SDFatFS.database;
	}		

  char *d_etr_p;	//pointer to entry struct
	uint8_t *dir_buf_p; //pointer to directory sector buffer
	uint8_t *data_p = data_buf;	//pointer to data buf
		
		_Bool nomore_fls = 0;
		
	dbgmsg("[%s] While...\r\n", __func__);
	
	while(!nomore_fls)
	{		
		if(sect_adr>=root_sectx) break; 
	if(SD_Driver.disk_read(SDFatFS.drv, dir_buf,f_sect,1)!=RES_OK)	//read 1 sector of folder
		{
						SDFile.err = FR_DISK_ERR;										//save error code to the struct
						errmsg("disk_read Failed\r\n"); 		//call error
						return FR_DISK_ERR;
						}		
		/*SEARCH  */
					for(uint8_t f=0;f<etrs_per_sect;f++)
					{
						if(nomore_fls) break;			//if no more files, exit
					dir_buf_p = &dir_buf[f*32];	//point at the entry
					d_etr_p = dir_entry.f_name;	//point at the start of struct		
						/*
						File attributes:
						ATTR_READ_ONLY 0x01
						ATTR_HIDDEN 0x02
						ATTR_SYSTEM 0x04
						ATTR_VOLUME_ID 0x08
						ATTR_DIRECTORY 0x10
						ATTR_ARCHIVE 0x20
						*/
					for(uint8_t j=0;j<32;j++)*d_etr_p++ = *dir_buf_p++; //copy folder entry	
						if((dir_entry.f_name[0]!='.')	//if filename's 1st character != 0x2E (subdir entry, there are usually 2 of them, . and ..)
								&&	((uint8_t)dir_entry.f_name[0]!=0xE5)	//!= 0xE5 (name used but the file deleted)
								&& ((uint8_t)dir_entry.attr <=0x20)				//attribute is correct
								&& ((uint8_t)dir_entry.f_ext[0]))					//extention not NULL
								//&&((dir_entry.fst_clu[0]+dir_entry.fst_clu[1])>1))	//start cluster>1
							{
								if(((uint8_t)dir_entry.attr !=0x10)&&((uint8_t)dir_entry.attr !=0x20))	continue;		//ultra tough files selections
								if(dir_entry.f_name[0]==0){nomore_fls = 1; break; }//if no more files, exit
								else if(!dir_entry.attr) {nomore_fls = 1; break; }
								
								_Bool dot =0;							//dot flag (set after ' ' or after 8 bytes of name passed)
								d_etr_p = dir_entry.f_name;	//point at the start of struct
								for(uint8_t j=0;j<8;j++)
								{
								if(*d_etr_p==' '){if(dir_entry.attr!=0x10){*data_p++ = '.'; dot = 1;} break;}
								*data_p++=*d_etr_p++;		
								}
								if(!dot) {*data_p++='.';}		//if no dot
								if(dir_entry.attr!=0x10)			//if not a folder
								{
								d_etr_p = dir_entry.f_ext;
								for(uint8_t j=0;j<3;j++)
								{
								*data_p++=*d_etr_p++;		
								}
								}
								uint32_t sz = 0;
								for(uint8_t x=4;x>0;x--){sz<<=8; sz+=dir_entry.f_size[x-1]; }
								sprintf((char*)data_p," %d B",sz);
								data_p =(uint8_t*) strstr((char*)data_p,"B");
								data_p++;
								*data_p++ = '\r';
								*data_p++ = '\n';		//new line					
							}//end if
					}//end for f
					f_sect +=1;	//to the next sector of folder												(+1 !!!!!!!!!!! not _MAX_SS!!!!!)
					sect_adr++;
		}//while
		if(notroot)	//close folder if it was open
	{
		dbgmsg("[%s] Closing...\r\n", __func__);
	res = f_closedir(&fold);
		if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_closedir FAILED\r\n"); 		//call error
		return res;
		}	
	
		dbgmsg("[%s] Closed...\r\n", __func__);
	}
	*data_p++ = '\0';
	return res;
}

/*
Write a string to a file
*/
FRESULT sd_f_swr(char *file_path,void *wr_buf, _Bool autoclose)
{
	FRESULT res;					//operation result
	uint32_t bytes_to_write=0;	//number of bytes to write
	uint8_t *bufptr = (uint8_t*)(wr_buf);//point to the buffer
	
	while(*bufptr != '\0')		//data size calculation (the end of file is '\0')
	{
		bufptr++;
		bytes_to_write++;
	}
	res = sd_f_write(file_path,wr_buf,bytes_to_write,autoclose);
	return res;
}
/*
Overwrite file with string
*/
FRESULT sd_f_sovr(char *file_path,void *wr_buf, _Bool autoclose)
{
	FRESULT res;					//operation result
	uint32_t bytes_to_write=0;	//number of bytes to write
	uint8_t *bufptr = (uint8_t*)(wr_buf);//point to the buffer
	
	while(*bufptr != '\0')		//data size calculation (the end of file is '\0')
	{
		bufptr++;
		bytes_to_write++;
	}
	res = sd_f_ovrt(file_path,wr_buf,bytes_to_write, autoclose);
	return res;
}

/*
Write a buffer of data to a file (doesn't affect existing data)
Input: char* file path of type "<folder> <'/'or'\\'> <file>"
			 void *wr_buf - buffer of data to be written into a file
			 uint32_t bytes_to_write - you got it
			autoclose - close file after writing
Return: FRESULT - operation result
*/
FRESULT sd_f_write(char *file_path,void *wr_buf,uint32_t bytes_to_write, _Bool autoclose)
{
	FRESULT res;					//operation result
	uint32_t bytes_written;	//number of written data
	uint8_t opn_fls;		//a pointer to the 'opened files list'

	//if the file's opened already
	if(sd_f_stat(file_path,&opn_fls) == SDFS_OPND) 	
	{
		SDFile = sd_opnd_files[opn_fls].fil_obj;
		f_info = sd_opnd_files[opn_fls].fil_info;
		res = f_write(&SDFile,wr_buf,bytes_to_write,&bytes_written);	//write the buffer of data into the file
	if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_write() Failed\r\n");
			return res;
		}
		f_stat(file_path,&f_info);
		sd_opnd_files[opn_fls].fil_obj = SDFile;
		sd_opnd_files[opn_fls].fil_info = f_info;
		return res;
	}
	//If the file isn't opened yet
	res = f_open(&SDFile,file_path,FA_WRITE | FA_OPEN_APPEND);		//open file and add the data
	if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_open() Failed\r\n");
			return res;
		}
		
	res = f_write(&SDFile,wr_buf,bytes_to_write,&bytes_written);	//write the buffer of data into the file
	if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_write() Failed\r\n");
			return res;
		}
	/*IF AUTOCLOSE*/	
		if(autoclose)
		{
			res =f_close(&SDFile);	//close file after writing
				if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_close() Failed\r\n");
			return res;
		}
		return res;
		}
		
	f_stat(file_path,&f_info);					//retrieve file information (name, ext, size..)
	f_cnt = SDFile.obj.lockid;			//if everything's fine , increment file counter	
		for(uint8_t f_idx=0;f_idx<_FS_LOCK;f_idx++)		//search for the empty struct object
		{
			if(sd_opnd_files[f_idx].fil_obj.obj.fs == 0) 
				{
				sd_opnd_files[f_idx].fil_info = f_info;
				sd_opnd_files[f_idx].fil_obj = SDFile;
				break;
				}			
		}

	return res;
}

/*
Overwrite a file with buffer (!erase all the existing data!)
Input: char* file path of type "<folder> <'/'or'\\'> <file>"
			 void *wr_buf - buffer of data to be written into a file
				autoclose - close file after overwriting
Return: FRESULT - operation result
*/
FRESULT sd_f_ovrt(char *file_path,void *wr_buf,uint32_t bytes_to_write, _Bool autoclose)
{
	FRESULT res;					//operation result
	uint32_t bytes_written;	//number of written data
	uint8_t opn_fls;		//a pointer to the 'opened files list'

	//if the file's opened already
	if(sd_f_stat(file_path,&opn_fls) == SDFS_OPND) 	
	{
		SDFile = sd_opnd_files[opn_fls].fil_obj;
		f_info = sd_opnd_files[opn_fls].fil_info;
		res = f_write(&SDFile,wr_buf,bytes_to_write,&bytes_written);	//write the buffer of data into the file
	if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_write() Failed\r\n");
			return res;
		}
		f_stat(file_path,&f_info);
		sd_opnd_files[opn_fls].fil_obj = SDFile;
		sd_opnd_files[opn_fls].fil_info = f_info;
		return res;
	}
	//If the file isn't opened yet
	res = f_open(&SDFile,file_path,FA_WRITE);		//open file and and overwrite
	if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_open() Failed\r\n");
			return res;
		}
		
	res = f_write(&SDFile,wr_buf,bytes_to_write,&bytes_written);	//write the buffer of data into the file
	if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_write() Failed\r\n");
			return res;
		}
		/*IF AUTOCLOSE*/	
		if(autoclose)
		{
			res =f_close(&SDFile);	//close file after writing
				if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_close() Failed\r\n");
			return res;
		}
		return res;
		}	
	f_stat(file_path,&f_info);					//retrieve file information (name, ext, size..)
	f_cnt = SDFile.obj.lockid;			//if everything's fine , increment file counter	
		for(uint8_t f_idx=0;f_idx<_FS_LOCK;f_idx++)		//search for the empty struct object
		{
			if(sd_opnd_files[f_idx].fil_obj.obj.fs == 0) 
				{
				sd_opnd_files[f_idx].fil_info = f_info;
				sd_opnd_files[f_idx].fil_obj = SDFile;
				break;
				}			
		}

	return res;
}

/*
Read a file from SD card
Input: file_path - char* file path of type "<folder> <'/'or'\\'> <file>"
			 file_buf - buffer to copy data into
			 bytes_to_read - FILE SIZE +1 OR MORE
				autoclose - close file after reading
Return: FRESULT - operation result
*/
FRESULT sd_f_read(char *file_path,void *file_buf, uint32_t buf_size, _Bool autoclose)
{
	uint32_t bytes_read=0;	//number of read bytes
	FRESULT res;					//operation result
	uint8_t *bufptr = (uint8_t*)(file_buf);//point to the buffer
	uint8_t opn_fls;		//a pointer to the 'opened files list'
	//if the file's already opened:
	if(sd_f_stat(file_path,&opn_fls) == SDFS_OPND) 	
	{
		SDFile = sd_opnd_files[opn_fls].fil_obj;
		f_info = sd_opnd_files[opn_fls].fil_info;
		/*OVRN DETECTION */
		if(buf_size<f_info.fsize+1)
		{
			res = FR_INVALID_PARAMETER;
			SDFile.err = res;										//save error code to the struct
		errmsg("SD RX_BUF OVERRUN\r\n");
		return res;
		}
			res = f_read(&SDFile,file_buf,f_info.fsize,&bytes_read);	//read file content into file_buf
		if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_read() Failed\r\n");
			return res;
		}
		
	bufptr+= f_info.fsize;				//clear the last buffer element
	if(*bufptr!='\0')	*bufptr = '\0';
	
		return res;
	}//end if(sd_f_stat...)
	//If the file's not opened yet:
	res = f_open(&SDFile,file_path,FA_READ);		//open file/folder and write to the FIL structure 
	if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_open() Failed\r\n");
			return res;
		}
	f_stat(file_path,&f_info);					//retrieve file information (name, ext, size..)
		
	/*OVRN DETECTION */
		if(buf_size<f_info.fsize+1)
		{
			res = FR_INVALID_PARAMETER;
			SDFile.err = res;										//save error code to the struct
		errmsg("SD RX_BUF OVERRUN\r\n");
			f_close(&SDFile);	//close file 
		return res;
		}
		
	res = f_read(&SDFile,file_buf,f_info.fsize,&bytes_read);	//read file content into file_buf
		if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_read() Failed\r\n");
			return res;
		}
		
	bufptr+= f_info.fsize;				//clear the last buffer element
	if(*bufptr!='\0')	*bufptr = '\0';
	
	/*IF AUTOCLOSE*/	
		if(autoclose)
		{
			res =f_close(&SDFile);	//close file after writing
				if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_close() Failed\r\n");
			return res;
		}
		return res;
		}	
	
	f_cnt = SDFile.obj.lockid;			//if everything's fine , increment file counter	
//	sd_opnd_files[f_cnt-1].fil_info = f_info;									//point file info struct to the current file
//	sd_opnd_files[f_cnt-1].fil_obj = SDFile;
		
		for(uint8_t f_idx=0;f_idx<_FS_LOCK;f_idx++)		//search for the empty struct object
		{
			if(sd_opnd_files[f_idx].fil_obj.obj.fs == 0) 
				{
				sd_opnd_files[f_idx].fil_info = f_info;
				sd_opnd_files[f_idx].fil_obj = SDFile;
				break;
				}			
		}
				
	return res;
}

/*EXPERIMENTAL*/

/*
Read a file by pieces
bytes_read - bytes already read (from the start of the file)
file is closing after reading
*/
FRESULT sd_f_seq_read (char *file_path,uint8_t *file_buf, uint32_t bytes_to_bite,uint32_t *bytes_read)
{
	FRESULT res;
	res = f_stat(file_path,&f_info);
	if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
		errmsg("f_stat() Failed\r\n"); 		//call error
		return res;
		}	
	
	uint8_t f_buf[_MAX_SS];							//temporary file buffer
	
	uint32_t f_sect;								//file sector (offset from the beginning of the file system) 
	uint32_t f_clust;								//file cluster (oofset from beginning of the file system)
	uint32_t f_csect;									//current file sector
	uint32_t f_cbyte =0;								//current file byte (in each sector)
  uint8_t *f_ptr;					//pointer to the byte of file
	_Bool cl_chg_flg = 0;			//cluster change flag
		
	uint8_t opn_fls;		//a pointer to the 'opened files list'
	//if the file's already opened:
	if(sd_f_stat(file_path,&opn_fls) == SDFS_OPND) 	
	{
		SDFile = sd_opnd_files[opn_fls].fil_obj;
		f_info = sd_opnd_files[opn_fls].fil_info;
			res = f_read(&SDFile,NULL,1,NULL);	//to get start of file ptr
		if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
		errmsg("f_read() Failed\r\n"); 		//call error
		return res;
		}
	}//end if(sd_f_stat...)
	else		//if file not open yet (or not exist)
	{
	/*OPEN FILE */
	res = f_open(&SDFile,file_path,FA_READ);		
		if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
		errmsg("f_open() Failed\r\n"); 		//call error
		return res;
		}	/*READ ENTRY */
	res = f_read(&SDFile,NULL,1,NULL);	//to get start of file ptr
		if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
		errmsg("f_read() Failed\r\n"); 		//call error
		return res;
		}	
	}
	/*DO MAGIC */
	uint16_t curr_bytes = 0;		//current bytes read
	f_cbyte =*bytes_read%_MAX_SS;		//obtain current index of the byte in current sector
	f_clust = SDFile.clust;			//set to start cluster
	
	f_sect = SDFile.sect;					//set to start of file sector
	f_sect+=*bytes_read/_MAX_SS;			//calculate sector of a file to read (address, NOT current sector of a file!!)	
		
	READ_SECTOR:	
	
	f_csect = f_sect - SDFile.sect;			//calculate current file sector (starting from beginning of the file, 0th)
		
	if(f_csect)
		{
			if(f_csect%SDFatFS.csize == 0)		//each time f_sect = 32, 64 , 96, 128 ... (cluster size in sectors)
			{
				cl_chg_flg = 1;			//go to next cluster
			}
		}	
		
	if(cl_chg_flg)	//if current cluster is fully read
	{
		//each sector in FAT has 256 cluster entries (512 byte-sector, 0-255)
		uint32_t fat_sect = SDFatFS.fatbase;	//obtain FAT starting sector
		fat_sect += f_clust/(_MAX_SS/2);			//calculate FAT sector to read
		
			if(SD_Driver.disk_read(SDFatFS.drv, f_buf,f_sect,1)!=RES_OK)	//read 1 sector of FAT
		{
						SDFile.err = FR_DISK_ERR;										//save error code to the struct
						errmsg("disk_read Failed\r\n"); 		//call error
						return FR_DISK_ERR;
						}	
		/*
		There is one more important computation related to the FAT. Given any valid cluster number N,
		where in the FAT(s) is the entry for that cluster number?
		If(FATType == FAT16)
		FATOffset = N * 2;
		Else if (FATType == FAT32)
		FATOffset = N * 4;
		*/
		uint16_t fat_sect_idx;
		if(SDFatFS.fs_type == FS_FAT16) fat_sect_idx= (f_clust*2)%_MAX_SS;		//calculate index in current FAT sector 
		else if(SDFatFS.fs_type == FS_FAT32) fat_sect_idx = (f_clust*4)%_MAX_SS;
		
		f_clust = f_buf[fat_sect_idx];				//find next cluster of file in FAT table (LOW byte)
		f_clust |= (f_buf[fat_sect_idx+1])<<8;	//HIGH byte of next cluster. now we have it
		if(SDFatFS.fs_type == FS_FAT32)
		{
			f_clust |= (f_buf[fat_sect_idx+2])<<16;	//for fat32
			f_clust |= (f_buf[fat_sect_idx+3])<<24;
			f_clust &= 0x0FFFFFFF;												//for FAT32, only 28 bits are valid for cluster entry
		}
		if(SDFatFS.fs_type == FS_FAT16)
		{
		if(f_clust>=0xFFF8) return FR_NO_FILE;			//end of file or bad cluster
			f_sect = (f_clust-1)*SDFatFS.csize + SDFatFS.dirbase;		//calculate next sector address of file
		}
		else if (SDFatFS.fs_type == FS_FAT32)
		{
			if(f_clust>=0x0FFFFFF8) return FR_NO_FILE;			//end of file or bad cluster
			f_sect = (f_clust-1)*SDFatFS.csize + SDFatFS.database;
		}
		
		else return FR_NO_FILESYSTEM;			//UNKNOWN FS
		
		cl_chg_flg = 0;
	}
	
		if(SD_Driver.disk_read(SDFatFS.drv, f_buf,f_sect,1)!=RES_OK)	//read 1 sector of a file
		{
						SDFile.err = FR_DISK_ERR;										//save error code to the struct
						errmsg("disk_read Failed\r\n"); 		//call error
						return FR_DISK_ERR;
						}	
	
	f_ptr = f_buf;
	f_ptr +=f_cbyte;		//current position in the temporary buffer
	
	for(uint16_t byte_idx = f_cbyte; byte_idx<_MAX_SS;byte_idx++)		//copy data within one sector
	{
		if(curr_bytes>=bytes_to_bite)	break;				//if needed less than there is in one sector, break
		if(*bytes_read>=f_info.fsize) return FR_OK;	//end of file
		*file_buf++ = f_buf[byte_idx];
		curr_bytes++;
		*bytes_read+=1;			//INCREMENT BYTE CTR
	}
	//end of reading sector
	if(curr_bytes<bytes_to_bite)		//if there's still left data to read
	{
		f_cbyte = 0;	//point at the first element of the next sector
		f_sect++;									//go to the next sector
		goto READ_SECTOR;		//read next sector
	}
	
	res = f_close(&SDFile);				//close file before exit
	if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
		errmsg("f_close() FAILED\r\n"); 		//call error
		return res;
		}	
		
	return FR_OK;
}

/*
Create a blank file or folder
Input: file path
			mode: can be a value of FA_READ, FA_WRITE (doesn't matter for folders)
			autoclose - close file after creating
Return: FRESULT - result of file creation (see ff.h for details)
*/
FRESULT sd_f_create (char *file_path, uint8_t mode, _Bool autoclose)
{
	FRESULT res;
	char fpcpy[strlen(file_path)];		//copy of the path
	_Bool fl_flag = 0;				//file flag
	char* sop = fpcpy;			//start of path
	
	for(uint8_t i=0;i<sizeof(fpcpy);i++)			//find out how many subdirectories to create
	{
		fpcpy[i] = file_path[i];			//copy the path
		
		if(fpcpy[i]=='.') {fl_flag = 1;}		//there is a '.' -> the user want to create  file
		if((fpcpy[i]=='/')||(fpcpy[i]=='\\'))	
			{
				fpcpy[i] = '\0';			//make a 'stop point' for the new subdir path
				res = f_mkdir(sop);		//create subdirectory
						if(res!= FR_OK) 												//if something went wrong
							{
								SDFile.err = res;										//save error code to the struct
							errmsg("f_mkdir() Failed\r\n");
							return res;
							}
				fpcpy[i] = '/';		//repair path
			}
	}
	
	if(!fl_flag){return FR_OK;}		//if there was no file, only folders, then return
	
	res = f_open(&SDFile,file_path,(FA_CREATE_NEW | mode));		//create new file
	if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_open() Failed\r\n");
			return res;
		}
		
		/*IF AUTOCLOSE*/	
		if(autoclose)
		{
			res =f_close(&SDFile);	//close file after creating
				if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_close() Failed\r\n");
			return res;
		}
		return res;
		}		
		
	f_stat(file_path,&f_info);					//retrieve file information (name, ext, size..)
		
	f_cnt = SDFile.obj.lockid;			//if everything's fine , increment file counter
		
	for(uint8_t f_idx=0;f_idx<_FS_LOCK;f_idx++)		//search for the empty struct object
		{
			if(sd_opnd_files[f_idx].fil_obj.obj.fs == 0) 
				{
				sd_opnd_files[f_idx].fil_info = f_info;
				sd_opnd_files[f_idx].fil_obj = SDFile;
				break;
				}			
		}
	
	return res;
}
//close file

FRESULT sd_f_close(char *file_path)
{
	FRESULT res;
	SDFSTAT st_res;
	uint8_t opn_fls;		//a pointer to the 'opened files list'
	st_res = sd_f_stat(file_path,&opn_fls);		//get file status
	
	if(st_res==SDFS_OPND)			//if file is opened
	{
		//SDFile = sd_opnd_files[opn_fls].fil_obj;
		//f_info = sd_opnd_files[opn_fls].fil_info;
		res = f_close(&sd_opnd_files[opn_fls].fil_obj);							//close the file
	if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_close() Failed\r\n");
			return res;
		}
		f_cnt--;			//decrement file counter
		//SDFile.obj.lockid = f_cnt;
		//sd_opnd_files[opn_fls].fil_obj = SDFile;
		sd_opnd_files[opn_fls].fil_info.fname[0] = NULL;	//clear file info
		sd_opnd_files[opn_fls].fil_info.fsize = NULL;
	return res;
	}//end if (st_res==SDFS_OPND)
	else return FR_OK;					//otherwise return (no opened file or file not found)
}
/*
Delete a file or a folder
*/
FRESULT sd_f_delete(char *file_path)
{
	FRESULT res;
	uint8_t opn_fls;
	if(sd_f_stat(file_path,&opn_fls) == SDFS_OPND) 			//if file is opened
	{
		SDFile.err = FR_DENIED;						//return error
		errmsg("sd_f_stat() Failed\r\n");
		return FR_DENIED;
	}
	//else
	res = f_unlink(file_path);	//delete file of directory
	if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_unlink() Failed\r\n");
			return res;
		}
		return res;
}

/*Get file status
Input: path - file path
Return: a value of SDFSTAT type (declared in sdcard.h):
				SDFS_NOPNYT				 		 File exists and not open yet 
				SDFS_OPND								File already opened
				SDFS_NOFL									No such file or directory

!!Works corect only if using sd_f_.. functions. 
If open a file with basic f_open(),the sd_file_info[] struct won't be filled and this function won't work

The function doesn't allow the two files with the same names to bo opened simultaneously (even if they're in different directories)
*/
SDFSTAT sd_f_stat(char* path,uint8_t* opnd_file)
{
	//opnd_file = NULL;
	SDFSTAT res;
	char *sop;							//start of path pointer
	char cpyfilename[11];	//array to copy open files' names into
	char cpypathname[11];
	uint8_t ccnt=0;	//char counter
	uint8_t len =0;
	sop = &path[ccnt];		//set the pointer to the beginning of the path

	while(path[ccnt]!= '\0')		//search the whole string for any '/'
	{
		if((path[ccnt]=='/') ||(path[ccnt]=='\\'))	{sop = &path[ccnt]; sop++;} 	//file name starts from here
		ccnt++;		//to the next char
	}
	cpypathname[0] = *sop;
	ccnt=0;
	while(*sop!= '\0')		//'a'->'A'
	{
		cpypathname[ccnt] = toupper(*sop++);
		ccnt++;		//to the next char
	}
	//Now when we've found the start of filename, we can compare it to the previously opened ones
	
	for(uint8_t i=0;i<_FS_LOCK;i++)
	{
		//look if we have the file already open
		
		char *fnameptr = sd_opnd_files[i].fil_info.fname;		//set the pointer at the beginning of the (potentionally) open file name
		char *cpyfnameptr = cpyfilename;				//set the pointer at the beginning of the file name copy
		
		while(*fnameptr!='\0')		//until the end of the filename
		{
			*cpyfnameptr++ = *fnameptr++;		//copy the name of the opened file into our cpy array
		}
		
		len = strlen(cpypathname);		//calculate the length of the file name from the input path
		if(!(strncmp(cpyfilename,cpypathname,len)))	//check if the file has been opened already
			{
			res = SDFS_OPND; //if it has, semaphore that
			*opnd_file = i;		//obtain the pointer to the existing file
			return res;}		//if we found that such file is already open, return
	}
	//if the file's not open, that means it either not exist or ready to open
	if(f_stat(path,NULL) !=FR_OK) {res = SDFS_NOFL; return res;}	//if there's no such file or path invalid, return NOFL
	else {res = SDFS_NOPNYT; return res; }	//if is's ready to open, return that
}


/*
FORMAT SD CARD
Input:
FAT_type - can be: 
					FS_FAT16 - for disks 8 MB - 4 GB
					FS_FAT32 - for disks >=256 MB
clu_size - cluster size for FAT (in sectors)
					FAT16:
							32 MB - 64 MB - 2 sect,
							64 MB - 128 MB - 4 sects,
							128 MB - 256 MB - 8 sects,
							256 MB - 512 MB - 16 sects,
							...
							1 GB - 2 GB - 64 sects,
							2 GB - 4 GB - 128 sects (?)
					FAT32:
							disk size 512 MB- 8 GB - 8 sectors,
							8 GB - 16 GB - 16 sectors	
							16 GB - 32 GB - 32 sectors,
							More - 64 sectors
!The numbers above are very relative, so you have to try!
(For example, 1GB Kingstone worked in FAT32 only with 16 sectors/cluster)
s
f_buf - buffer to format the card (the bigger - the better and faster)
f_buf_size - size of buffer (better use buffer of at least 1 sector (512 bytes default, otherwise - _MAX_SS))
*/
FRESULT sd_format(uint8_t FAT_type, uint8_t clu_size, uint8_t *f_buf, uint32_t f_buf_size)
{
	FRESULT res = f_mkfs(SDPath,FAT_type,_MAX_SS*clu_size,f_buf,f_buf_size);
	if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
		errmsg("f_mkfs() Failed\r\n"); 		//call error
		return res;
		}
	res =	f_mount(&SDFatFS,SDPath,SD_MOUNT_NOW);		//mount disk
		if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
		errmsg("f_mount() Failed\r\n"); 		//call error
		return res;
		}
	return res;
}

/*HTTP HEADER*/
SDFSTAT add_http_header(char *file_path,uint8_t *filedata, uint8_t *out_data, uint32_t* out_len)
{
	uint8_t file_idx;
	SDFSTAT res = sd_f_stat(file_path,&file_idx);
	if(res!= SDFS_OPND) 												//if the file is not opened
		{
			SDFile.err = FR_LOCKED;										//save error code to the struct
			errmsg("sd_f_stat() Failed\r\n");
			return res;
		}
uint32_t fsz = sd_opnd_files[file_idx].fil_info.fsize;
uint8_t ascii_digs = 0;

while(fsz)
{
	fsz=fsz/10;
	ascii_digs++;	//calculate number of digits to display f size
}	
		
fsz = sd_opnd_files[file_idx].fil_info.fsize;
char fsz_ascii[ascii_digs];
//Translate file size to ascii:
char rmd;	//remainder 

while (fsz>=10){	
ascii_digs--;
rmd=fsz%10;		
fsz=fsz/10;		
fsz_ascii[ascii_digs]=rmd+'0';	
}//end while
 fsz_ascii[0]=fsz+'0';	//
fsz = sd_opnd_files[file_idx].fil_info.fsize;
		
char http_h[] =" \
/* HTTP header */\r\n\
/* 'HTTP/1.1 200 OK \r\n\
'(17 bytes) */\r\n \
0x48,0x54,0x54,0x50,0x2f,0x31,0x2e,0x31,0x20,0x32,0x30,0x30,0x20,0x4f,0x4b,0x0d,0x0a,\r\n\
/* 'Server: lwIP/1.3.1 (http://savannah.nongnu.org/projects/lwip)\r\n\
'(63 bytes) */\r\n\
0x53,0x65,0x72,0x76,0x65,0x72,0x3a,0x20,0x6c,0x77,0x49,0x50,0x2f,0x31,0x2e,0x33,\r\n\
0x2e,0x31,0x20,0x28,0x68,0x74,0x74,0x70,0x3a,0x2f,0x2f,0x73,0x61,0x76,0x61,0x6e,\r\n\
0x6e,0x61,0x68,0x2e,0x6e,0x6f,0x6e,0x67,0x6e,0x75,0x2e,0x6f,0x72,0x67,0x2f,0x70,\r\n\
0x72,0x6f,0x6a,0x65,0x63,0x74,0x73,0x2f,0x6c,0x77,0x69,0x70,0x29,0x0d,0x0a,\r\n\
/* 'Content-Length: %s\r\n\
'(18+ bytes) */\r\n\
0x43,0x6f,0x6e,0x74,0x65,0x6e,0x74,0x2d,0x4c,0x65,0x6e,0x67,0x74,0x68,0x3a,0x20,\r\n\
0x33,0x34,0x39,0x36,0x39,0x0d,0x0a,\r\n\
/* 'Connection: Close \r\n\
'(19 bytes) */\r\n\
0x43,0x6f,0x6e,0x6e,0x65,0x63,0x74,0x69,0x6f,0x6e,0x3a,0x20,0x43,0x6c,0x6f,0x73,\r\n\
0x65,0x0d,0x0a,\r\n\
/* 'Content-type: text/plain\r\n\
'(28 bytes) */\r\n\
0x43,0x6f,0x6e,0x74,0x65,0x6e,0x74,0x2d,0x74,0x79,0x70,0x65,0x3a,0x20,0x74,0x65,\r\n\
0x78,0x74,0x2f,0x70,0x6c,0x61,0x69,0x6e,0x0d,0x0a,0x0d,0x0a,/* raw file data (%s bytes)*/\r\n\
// File's data here\r\n%s";

	sprintf((char*)out_data,http_h,fsz_ascii,fsz_ascii,(char*)filedata);

//*out_len = fsz;
*out_len =strlen(http_h);		//calculate header length
*out_len += (strlen(fsz_ascii)*2);	//add 2 strings of size
*out_len -=4;										//minus 2 %s
*out_len +=fsz;		//add file size
return res;
}

