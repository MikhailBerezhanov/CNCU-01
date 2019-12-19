/**
  ******************************************************************************
  * File Name          : sdcard.c
  * Description        : Wrap of FATFS over SDIO driver. This module uses DMA, 
													queue and semaphore to work under freeRTOS.
													(refer to sd_diskio.c for init call)
  ******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "sdcard.h"

#define DEBUG_MSG DEBUG_SD
#include "dbgmsg.h"

SD_HandleTypeDef hsd;
DMA_HandleTypeDef hdma_sdio_rx;
DMA_HandleTypeDef hdma_sdio_tx;
char SDPath[4];   						/* SD logical drive path */
FATFS SDFatFS;    						/* File system object for SD logical drive */
FIL SDFile;       						/* File object for SD */
FILINFO f_info;
OFLIST sd_opnd_files[_FS_LOCK];		//list of opened files
FILINFO sd_file_info[_FS_LOCK];	//struct array to store file-specific info (number of files determined by _FS_LOCK in ffconf.h)
uint8_t f_cnt=0;			//opened files counter 
uint8_t sector_buf[_MAX_SS];					//temporary buffer for sd blocks

void HAL_SD_MspInit(SD_HandleTypeDef* sdHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(sdHandle->Instance==SDIO)
  {
    /* SDIO clock enable */
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

    /* SDIO DMA Init */
    /* SDIO_RX Init */
    hdma_sdio_rx.Instance = DMA2_Stream3;
    hdma_sdio_rx.Init.Channel = DMA_CHANNEL_4;
    hdma_sdio_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_sdio_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_sdio_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_sdio_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_sdio_rx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_sdio_rx.Init.Mode = DMA_PFCTRL;
    hdma_sdio_rx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_sdio_rx.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    hdma_sdio_rx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    hdma_sdio_rx.Init.MemBurst = DMA_MBURST_INC4;
    hdma_sdio_rx.Init.PeriphBurst = DMA_PBURST_INC4;
    if (HAL_DMA_Init(&hdma_sdio_rx) != HAL_OK)
    {
      errmsg("HAL_DMA_Init SDIO RX failed\r\n");
    }

    __HAL_LINKDMA(sdHandle, hdmarx, hdma_sdio_rx);

    /* SDIO_TX Init */
    hdma_sdio_tx.Instance = DMA2_Stream6;
    hdma_sdio_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_sdio_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_sdio_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_sdio_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_sdio_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    hdma_sdio_tx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    hdma_sdio_tx.Init.Mode = DMA_PFCTRL;
    hdma_sdio_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_sdio_tx.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    hdma_sdio_tx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    hdma_sdio_tx.Init.MemBurst = DMA_MBURST_INC4;
    hdma_sdio_tx.Init.PeriphBurst = DMA_PBURST_INC4;
    if (HAL_DMA_Init(&hdma_sdio_tx) != HAL_OK)
    {
      errmsg("HAL_DMA_Init SDIO TX failed\r\n");
    }

    __HAL_LINKDMA(sdHandle, hdmatx, hdma_sdio_tx);

    /* SDIO interrupt Init */
    HAL_NVIC_SetPriority(SDIO_IRQn, SD_Global_PreP, SD_Global_SubP);
    HAL_NVIC_EnableIRQ(SDIO_IRQn);
  }
}

void HAL_SD_MspDeInit(SD_HandleTypeDef* sdHandle)
{
  if(sdHandle->Instance==SDIO)
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

    /* SDIO DMA DeInit */
    HAL_DMA_DeInit(sdHandle->hdmarx);
    HAL_DMA_DeInit(sdHandle->hdmatx);

    /* SDIO interrupt Deinit */
    HAL_NVIC_DisableIRQ(SDIO_IRQn);
  }
}



/* File system driver linking. */
static uint8_t FATFS_Init(void) 
{
	uint8_t retSD = 0;
  /*## FatFS: Link the SD driver ###########################*/
  if ((retSD = FATFS_LinkDriver(&SD_Driver, SDPath)) != 0)
		errmsg("FATFS_LinkDriver() Error: %d\r\n", retSD);
	
	return retSD;
}

/* SDIO init function */
uint8_t SD_DMA_Init(void)
{
  hsd.Instance = SDIO;
  hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
  hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
  hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
  hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
  hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd.Init.ClockDiv = 0;

	/* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, SD_DMA_RX_PreP, SD_DMA_RX_SubP);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
  /* DMA2_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, SD_DMA_TX_PreP, SD_DMA_TX_SubP);
  HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
	
	return FATFS_Init();	
}

/* SDIO deinit function */
void SD_DMA_Reinit(void)
{
	FATFS_UnLinkDriver(SDPath);
	memset(&SDFatFS, 0, sizeof(FATFS));
	FATFS_Init();
	
	sd_remount();
}

FRESULT sd_mount(void)
{
	FRESULT res = FR_OK;
	
	if ((res = f_mount(&SDFatFS, (TCHAR const*)SDPath, 0)) != FR_OK)
	{
		errmsg("f_mount Error: %d\r\n", res);
	}

	return res;
}

FRESULT sd_unmount(void)
{
	FRESULT res = FR_OK;
	
	if ((res = f_mount(NULL, (TCHAR const*)SDPath, 0)) != FR_OK)
	{
		errmsg("f_mount Error: %d\r\n", res);
	}

	return res;
}

FRESULT sd_remount(void)
{
	if ((sd_unmount() != FR_OK) || (sd_mount() != FR_OK))
	{
		errmsg("SD remount failed\r\n");
		return FR_NOT_READY;
	}
	
	return FR_OK;
}



FRESULT sd_f_read_(const char* fname, void *buf, size_t buf_size, _Bool unused)
{
	UINT bytesread = 0;
	FRESULT res = FR_OK;
	
	if ((res = f_open(&SDFile, fname, FA_READ)) != FR_OK) 
	{
		errmsg("f_open failed: %d\r\n", res);
	}
  else
	{
		res = f_read(&SDFile, buf, buf_size, &bytesread);
		if((bytesread == 0) || (res != FR_OK))
		{
				errmsg("Error file read: %d . Bytesread:%d\r\n", res, bytesread);
		}

		f_close(&SDFile);
	}
		
	return res;
}

/*
Get SD-card specs
*/
void sd_get_info(uint8_t *data_buf)
{
	if(hsd.State != HAL_SD_STATE_READY) 
	{return;}
	//uint8_t *data_buf_ptr = data_buf;
				switch(hsd.SdCard.CardType)				
				{
					case CARD_SDSC: sprintf((char*)data_buf,"SD CARD:SDSC\r\n"); break;
				
					case CARD_SDHC_SDXC: sprintf((char*)data_buf,"SD CARD:SDHC/XC\r\n"); break;
				
					case CARD_SECURED: sprintf((char*)data_buf,"!SD CARD TYPE SECURED!\r\n"); break;
					
					default: sprintf((char*)data_buf,"!UNKNOWN TYPE!\r\n");	return;
				}					
				
				switch(hsd.SdCard.CardVersion)
				{
					case CARD_V1_X: strcat((char*)data_buf,"VERSION: V1_X\r\n"); break;
					
					case CARD_V2_X: strcat((char*)data_buf,"VERSION: V2_X\r\n"); break;
				}
				switch(SDFatFS.fs_type)
				{
					case FS_FAT12: strcat((char*)data_buf,"FS: !FAT12!\r\n"); break;
					case FS_FAT16: strcat((char*)data_buf,"FS: FAT16\r\n"); break;
					case FS_FAT32: strcat((char*)data_buf,"FS: FAT32\r\n"); break;
					case FS_EXFAT: strcat((char*)data_buf,"FS: !EXFAT!\r\n"); break;
					default: strcat((char*)data_buf,"FS: UNKNOWN!\r\n");
				}	
				
				char str[64];
				sprintf(str,"BLOCKS:%d\r\nBLOCK SZ:%d\r\nTOTAL CAPACITY:%d MB\r\n",	\
				hsd.SdCard.BlockNbr,\
				hsd.SdCard.BlockSize,\
				(hsd.SdCard.BlockSize*hsd.SdCard.BlockNbr)/1000000);	//end sprintf
				strcat ((char*)data_buf,str);
}
//Scan ROOT dir for files and folders (FAT16,FAT32)
//return total number of files and folders on the disk
//			 Diskio_drvTypeDef *SD_Drv - structure object for SD-specific functions(SD_initialize,SD_status,SD_read, etc)
//			 FATFS *FAT_stuct - struct object for FAT (FAT type, size, cluster size,etc)
//Return: uint32_t number of files/folders on disk
uint32_t sd_n_files(void)
{
	const Diskio_drvTypeDef *SD_Drv = &SD_Driver;		//appeal to default driver defined in sd_diskio.c
	FATFS *FAT_struct = &SDFatFS;							//defined in fatfs.c
	//uint8_t sector_buf[_MAX_SS];					//temporary buffer for sd blocks
				
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
		//uint16_t fat16_sbuf[fat16_bufsz];
		uint16_t fat16_entry;									//16-bit variable for FAT entry
		while(!end)
			{
				if(SD_Drv->disk_read(FAT_struct->drv,sector_buf,(FAT_struct->fatbase)+sect_adr*_MAX_SS,1)!=RES_OK) 		//read 1 sector of FAT
				{
					SDFile.err = FR_DISK_ERR;										//save error code to the struct
					errmsg("disk_read error: %d\r\n", SDFile.err);
					return 0;
				}	

				// 0xFFF8		
				if(!sect_adr) {i=2;} else{i=0;}		//skip the first entry of FAT16 (0xFFF8)
				for( ;i<fat16_bufsz;i++)
				{
					fat16_entry = sector_buf[i*4];
					fat16_entry |= sector_buf[i*4+1]<<8;
					if(clust> FAT_struct->last_clst) return f_cnt;		//if last occupied cluster reached
					if(fat16_entry>=0xFFF8) {f_cnt++;}
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
	//	uint32_t fat32_sbuf[fat32_bufsz];
		uint32_t fat32_entry;									//32-bit variable for FAT entry
		while(!end)
			{
				if(SD_Drv->disk_read(FAT_struct->drv,sector_buf,(FAT_struct->fatbase)+sect_adr*_MAX_SS,1)!=RES_OK) 		//read 1 sector of FAT
				{
					SDFile.err = FR_DISK_ERR;										//save error code to the struct
					errmsg("disk_read error: %d\r\n", SDFile.err);
					return 0;
				}
				
				// 0x0FFFFFF8		
				if(!sect_adr) {i=2;} else{i=0;}		//skip the first entry of FAT32 (0xFFFFFFF8)
				for( ;i<fat32_bufsz;i++)
				{
					fat32_entry = sector_buf[i*4];					//transform 8-bit entries into 32-bit
					fat32_entry |= sector_buf[i*4+1]<<8;
					fat32_entry |= sector_buf[i*4+2]<<16;
					fat32_entry |= sector_buf[i*4+3]<<24;
					
					if(clust> FAT_struct->last_clst) return f_cnt;	//if last occupied cluster reached
					if((fat32_entry & 0x0FFFFFFF)>=0x0FFFFFF8) {f_cnt++;}
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
NO UNICODE
return list of files in folder into the buffer (and total number of files in that folder/directory)
each file takes about 20 bytes (SFN). So if there's lots of files, use bigger buffer 
(if using LFN, you must provide a real large buffer (~50 bytes for each file, depending on name length and size)
Input:
	*dir_path - path to search in (<"FOLDER/SUBFOLDER"> or whatever. Enter NULL to search in ROOT)
	*data_buf - pointer to the buffer where files' list appears
	buf_size - size of buffer (the bigger the better)
	*file_cnt - pointer to the variable to contain the number of files in current directory. can be NULLed
Output:
		FRESULT - operation result (0 - ok, others - not ok, see ff.h)
*/
FRESULT sd_dir_files_list (const char *dir_path, uint8_t *data_buf, uint32_t buf_size , uint32_t *file_cnt)
{
	const Diskio_drvTypeDef *SD_Drv = &SD_Driver;
	FATFS *FAT_struct = &SDFatFS;
	FRESULT res = FR_INVALID_PARAMETER;		//invalid parameter = buffer is too small
	*file_cnt = 0;					//clear file counter
	//uint32_t f_total = sd_n_files();						//calculate total files on disk
	/*
	if(buf_size < f_total * 20) 			//quite a dumb check, but who cares if it works
	{
			SDFile.err = res;										//save error code to the struct
		_Error_Handler(__FILE__,__LINE__); 		//call error
		return res;
	}
	*/
	//uint8_t dir_buf[_MAX_SS];					//temporary buffer for sd blocks
	uint32_t sect_adr =0;		//current sector of folder
	uint8_t etrs_per_sect = _MAX_SS/32;		//sector size / entry size = entries per sector
	uint32_t root_entries = FAT_struct->last_clst * FAT_struct->csize;	//max entries (number of occupied clusters * cluster size = occupied sectors)
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
	
#if _USE_LFN				//if long file names used (defined in ffconf.h)
	
	struct lfn_str
	{
		char seq_nmbr[1];		//sequence number and allocation status
		char f_name1[10];	//file name part (UNICODE, 16 bit per char!)
		char attr;			//file attribute
		char res1;			//reserved
		char chksm;			//LFN checksum
		char f_name2[12];	//file name part2 (UNICODE, 16 bit per char!)
		char res2[2];		//reserved
		char f_name3[4];	//file name part 3 (UNICODE, 16 bit per char!)
	}__attribute((packed)) lfn_entry;
	
char *lfn_entry_p;			//pointer to lfn entry struct
WCHAR *lfn_temp_p;				//pointer to the temp lfn buffer (16-bit WCHAR names, see LfnBuf in ff.c)
uint8_t lfn_etrx = 0;					//total number of LFN entries (to be revealed in process)	
uint8_t checksum=0;					//checksum to match SFN entries with LFNs
#endif
	
	uint32_t f_sect;				//current sector address
if(dir_path) 							//if not root
{	
	res = f_opendir(&fold,dir_path);		//open directory (to obtain sector address)
	if(res!= FR_OK) 												//if something went wrong
	{
		SDFile.err = res;										//save error code to the struct
		errmsg("f_opendir error: %d\r\n", SDFile.err);
	return res;
	}	
f_sect	= fold.sect;		//obtain sector address of the folder
notroot = 1;							// mark directory as no root
}
else 							//if root folder
	{
	if(FAT_struct->fs_type == FS_FAT16) f_sect = FAT_struct->dirbase;
	else if(FAT_struct->fs_type == FS_FAT32) f_sect = FAT_struct->database;
	}		

  char *d_etr_p;	//pointer to entry struct
	uint8_t *dir_buf_p; //pointer to directory sector buffer
	uint8_t *data_p = data_buf;	//pointer to data buf
		
		_Bool nomore_fls = 0;
		
	while(!nomore_fls)
	{		
		if(sect_adr>=root_sectx) break; 
	if(SD_Drv->disk_read(FAT_struct->drv, sector_buf,f_sect,1)!=RES_OK)	//read 1 sector of folder
		{
			SDFile.err = FR_DISK_ERR;										//save error code to the struct
			errmsg("disk_read error: %d\r\n", SDFile.err);
			return FR_DISK_ERR;
		}		
		/*SEARCH  */
					for(uint8_t f=0;f<etrs_per_sect;f++)
					{
						if(nomore_fls) break;			//if no more files, exit
					dir_buf_p = &sector_buf[f*32];	//point at the entry
					d_etr_p = dir_entry.f_name;	//point at the start of struct		
						/*
						File attributes:
						*/
					#define	ATTR_READ_ONLY 0x01
					#define	ATTR_HIDDEN 0x02
					#define	ATTR_SYSTEM 0x04
					#define	ATTR_VOLUME_ID 0x08
					#define	ATTR_DIRECTORY 0x10
					#define	ATTR_ARCHIVE 0x20
						
					#define	LFN_ATTR 0x0F
						
						
					for(uint8_t j=0;j<32;j++)*d_etr_p++ = *dir_buf_p++; //copy folder entry	
						if((dir_entry.f_name[0]!='.')	//if filename's 1st character != 0x2E (subdir entry, there are usually 2 of them, . and ..)
								&&	((uint8_t)dir_entry.f_name[0]!=0xE5)	//!= 0xE5 (name used but the file deleted)
								&& ((uint8_t)dir_entry.attr <=0x20)				//attribute is correct
								/*&& ((uint8_t)dir_entry.f_ext[0])*/)					//extention not NULL
								//&&((dir_entry.fst_clu[0]+dir_entry.fst_clu[1])>1))	//start cluster>1
							{
								
								if(dir_entry.f_name[0]==0){nomore_fls = 1; break; }//if no more files, exit
								else if(!dir_entry.attr) {nomore_fls = 1; break; }	//if attribute = 0
								
								if(((uint8_t)dir_entry.attr !=ATTR_DIRECTORY)		//ultra tough files selection (also checks LFN entries)
								&&((uint8_t)dir_entry.attr !=ATTR_ARCHIVE)
								&&((uint8_t)dir_entry.attr !=ATTR_READ_ONLY)
								&&((uint8_t)dir_entry.attr !=ATTR_HIDDEN)
								&&((uint8_t)dir_entry.attr !=LFN_ATTR)
								&&((uint8_t)dir_entry.attr !=ATTR_SYSTEM))	
								continue;		
								
#if _USE_LFN			//IF LONG FILE NAMES USED

								if((uint8_t)dir_entry.attr == 0x0F)				//if LFN entry detected and permitted
								{
									d_etr_p = dir_entry.f_name;	//point at the start of struct
									lfn_entry_p = lfn_entry.seq_nmbr;		//point at the start of lfn entry struct
									for(uint8_t j=0;j<32;j++)*lfn_entry_p++ = *d_etr_p++; //copy previously obtained entry into lfn entry struct
									
									if(lfn_entry.seq_nmbr[0] & 0x40)				//if it's the last lfn entry for this file (the upper one)
									{
										lfn_etrx = (uint8_t)(lfn_entry.seq_nmbr[0] - 0x40);			//retrieve total number of lfn entries (will be 1 or more)
										//lfn_temp_p = lfn_temp;								//point to the start of lfn temp buffer
										lfn_temp_p = FAT_struct->lfnbuf;	//16-bit pointer
										//lfn_temp_p += (lfn_etrx-1)*26;						//shift pointer by 26*total_lfn_entries (each lfn entry contain 26 bytes of file name (13 UNICODE symbols)
																													//(thus the file name contained in the entry with 0x40 mark appears in the end of temp lfn buffer)
										lfn_temp_p += (lfn_etrx-1)*13;
										lfn_etrx--;			//decrement number of entries remained 
									}//end if 0x40
									
									else			//if that's not the first lfn entry
									{
										//lfn_etrx = (uint8_t)(lfn_entry.seq_nmbr[0]);		//retrieve current entry's number
										//lfn_temp_p = lfn_temp;								//point to the start of lfn temp buffer
										lfn_temp_p = FAT_struct->lfnbuf;	//16-bit pointer
										//lfn_temp_p += (uint8_t)(lfn_entry.seq_nmbr[0]-1)*26;						//shift pointer by 26*total_lfn_entries
										lfn_temp_p += (lfn_entry.seq_nmbr[0]-1)*13;
										lfn_etrx--;			//decrement number of entries remained
									}//end else
									
										/*COPY 3 PARTS OF FILENAME CONTAINED IN LFN ENTRY*/
										
										lfn_entry_p = lfn_entry.f_name1;			//point at the 1 part of name field of current entry
										for(uint8_t j=0;j<sizeof(lfn_entry.f_name1)/sizeof(WCHAR);j++) 
																		{
																		*lfn_temp_p = *lfn_entry_p++;
																		*lfn_temp_p++ |= (*lfn_entry_p++)<<8;
																		}
										lfn_entry_p = lfn_entry.f_name2;			//point at the 2 part of name field of current entry
										for(uint8_t j=0;j<sizeof(lfn_entry.f_name2)/sizeof(WCHAR);j++) //{*lfn_temp_p++ = *lfn_entry_p++;}
																		{
																		*lfn_temp_p = *lfn_entry_p++;
																		*lfn_temp_p++ |= (*lfn_entry_p++)<<8;
																		}
										lfn_entry_p = lfn_entry.f_name3;			//point at the 3 part of name field of current entry
										for(uint8_t j=0;j<sizeof(lfn_entry.f_name3)/sizeof(WCHAR);j++) //{*lfn_temp_p++ = *lfn_entry_p++;}
																		{
																		*lfn_temp_p = *lfn_entry_p++;
																		*lfn_temp_p++ |= (*lfn_entry_p++)<<8;
																		}
										if(!lfn_etrx)			//if all parts of lfn filename has been copied to the temp lfn buffer
										{
											//lfn_temp_p = lfn_temp;								//point to the start of lfn temp buffer
											lfn_temp_p = FAT_struct->lfnbuf;	//16-bit pointer
											_Bool lfn_eos = 0;									//end of string marker for lfn name
											
											/*COPY LFN NAME.EXT (ASCII/ANSI ONLY) */
											while(!lfn_eos)
											{
											if(*lfn_temp_p==0x0000)		//if found the end of string
												{
												//	if((uint8_t)*(++lfn_temp_p)==0xFF)	//..and the next byte is also 0xFF
												//	{
														lfn_eos = 1;				//stop copying, exit 
														break;
												//	}
												//	else {lfn_temp_p--;}	//if the second byte is not 0xFF, return to the previous position
												}
											else if (*lfn_temp_p==0xFFFF)		//just in case..
											{
												lfn_eos = 1;				//stop copying, exit 
												break;
											}
											*data_p++ = (uint8_t)(*lfn_temp_p++);	//copy file name (extension included) into user buffer
										//	lfn_temp_p++;								//skip trailing 0x00s of UNICODE, use 1-byte ANSI chars only
											}//end while
										}//end if(!lfn_etrx)
										
										continue;					//go to next entry
								}//end if 0x0F
								
#else
								if((uint8_t)dir_entry.attr == 0x0F)				//if LFN are not permitted
								{
									continue;				//skip this entry
								}
#endif								
								
								_Bool lfn_match = 0;			//used to indicate that current SFN entry has its LFN match 
								_Bool dot =0;							//dot flag (set after ' ' or after 8 bytes of name passed)
								d_etr_p = dir_entry.f_name;	//point at the start of entry struct
								
#if _USE_LFN			//IF LONG FILE NAMES USED
								//Calculate checksum
								checksum = 0;
								for(uint8_t j=0;j<11;j++)			
								{
									checksum = ( (checksum>>1) | ((checksum&1)<<7) ) + *d_etr_p++;	//RoR checksum + current name/ext char
								}
								if(lfn_entry.chksm == checksum)	//check if we already have file name from LFN entry
								{lfn_match = 1;}						//indicate the file has already been observed
								else {d_etr_p = dir_entry.f_name;}	//point at the start of entry struct (if SFN entry doesn't match LFN)
#endif
							if(!lfn_match)		//do following only if it's an SFN file
							{
								for(uint8_t j=0;j<8;j++)
								{														//copy name
								if(*d_etr_p==' '){if(dir_entry.attr!=0x10){*data_p++ = '.'; dot = 1;} break;}
								*data_p++=*d_etr_p++;		
								}
								if(!dot) {if(dir_entry.attr!=0x10) *data_p++='.';}		//if no dot and it's not a folder
								if(dir_entry.attr!=0x10)			//if not a folder
								{
								d_etr_p = dir_entry.f_ext;	
								for(uint8_t j=0;j<3;j++)
								{
								*data_p++=*d_etr_p++;			//copy extention
								}
								}//end if attr!=0x10
							}
							/*SIZE*/
								uint32_t sz;							
								sz = 0;
							if(dir_entry.attr!=0x10)		//if it's not a folder
							{
								for(uint8_t x=4;x>0;x--){sz<<=8; sz+=dir_entry.f_size[x-1]; }		//file size
								//Size conversion
								if(sz>1024)
								{
									sz/=1024;
									sprintf((char*)data_p," %d KB",sz);
								}
								else if(sz>0x100000)
								{
									sz/=100000;
									sprintf((char*)data_p," %d MB",sz);
								}
								else
								{
								sprintf((char*)data_p," %d B",sz);
								}
								
								data_p =(uint8_t*) strstr((char*)data_p,"B");
								data_p++;
							}//if not folder
								*data_p++ = '\r';
								*data_p++ = '\n';		//new line				
								
								*file_cnt +=1;
							}//end if
					}//end for f
					f_sect +=1;	//to the next sector of folder												(+1 !!!!!!!!!!! not _MAX_SS!!!!!)
					sect_adr++;
		}//while
	if(notroot)	//close folder if it was open
	{
	res = f_closedir(&fold);
		if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_closedir error: %d\r\n", SDFile.err);
			return res;
		}	
	}
	*data_p++ = '\0';
	return res;
}

/*
Write a string to a file
*/
FRESULT sd_f_swr(const char *file_path, void *wr_buf, _Bool autoclose)
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
FRESULT sd_f_sovr(const char *file_path,void *wr_buf, _Bool autoclose)
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
FRESULT sd_f_write(const char *file_path, void *wr_buf,uint32_t bytes_to_write, _Bool autoclose)
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
			errmsg("f_write Error: %d\r\n", res);
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
			errmsg("f_open Error: %d\r\n", res);
			return res;
		}
		
	res = f_write(&SDFile,wr_buf,bytes_to_write,&bytes_written);	//write the buffer of data into the file
	if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_write Error: %d\r\n", res);
			return res;
		}
	/*IF AUTOCLOSE*/	
		if(autoclose)
		{
			res =f_close(&SDFile);	//close file after writing
				if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_close Error: %d\r\n", res);
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
FRESULT sd_f_ovrt(const char *file_path,void *wr_buf,uint32_t bytes_to_write, _Bool autoclose)
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
			errmsg("f_write Error: %d\r\n", res);
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
			errmsg("f_open Error: %d\r\n", res);
			return res;
		}
		
	res = f_write(&SDFile,wr_buf,bytes_to_write,&bytes_written);	//write the buffer of data into the file
	if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_write Error: %d\r\n", res);
			return res;
		}
		/*IF AUTOCLOSE*/	
		if(autoclose)
		{
			res = f_close(&SDFile);	//close file after writing
			if(res!= FR_OK) 												//if something went wrong
			{
				SDFile.err = res;										//save error code to the struct
				errmsg("f_close Error: %d\r\n", res);
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
			buf_size - FILE SIZE +1 OR MORE
			autoclose - close file after reading
Return: FRESULT - operation result
*/
FRESULT sd_f_read(const char *file_path,void *file_buf, uint32_t buf_size, _Bool autoclose)
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
			errmsg("buf_size Error: %d\r\n", res);
			return res;
		}
			res = f_read(&SDFile,file_buf,f_info.fsize,&bytes_read);	//read file content into file_buf
		if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_read Error: %d\r\n", res);
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
			errmsg("f_open Error: %d\r\n", res);
			return res;
		}
	f_stat(file_path,&f_info);					//retrieve file information (name, ext, size..)
	/*OVRN DETECTION */
		if(buf_size<f_info.fsize+1)
		{
			res = FR_INVALID_PARAMETER;
			SDFile.err = res;										//save error code to the struct
			errmsg("buf_size Error: %d\r\n", res);
			f_close(&SDFile);	//close file 
			return res;
		}
	res = f_read(&SDFile,file_buf,f_info.fsize,&bytes_read);	//read file content into file_buf
		if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_read Error: %d\r\n", res);
			return res;
		}
		
	bufptr+= f_info.fsize;				//clear the last buffer element
	if(*bufptr!='\0')	*bufptr = '\0';
	
	/*IF AUTOCLOSE*/	
		if(autoclose)
		{
			res = f_close(&SDFile);	//close file after writing
				if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_close Error: %d\r\n", res);
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
file is closing after each reading
*/
int sd_f_seq_read (const char *file_path, void *buf, uint32_t bytes_to_bite, uint32_t *bytes_read)
{
	FRESULT res;
	BYTE *file_buf = (BYTE*)buf;
	
	// uint8_t f_buf[_MAX_SS];							//temporary file buffer
	
	uint32_t f_sect;								//file sector (offset from the beginning of the file system) 
	uint32_t f_clust;								//file cluster (oofset from beginning of the file system)
	uint32_t f_csect;									//current file sector
	uint32_t f_cbyte =0;								//current file byte (in each sector)
  uint8_t *f_ptr;					//pointer to the byte of file
	_Bool cl_chg_flg = 0;			//cluster change flag
  
/*	
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
_Error_Handler(__FILE__,__LINE__); 		//call error
return res;
			}
	}//end if(sd_f_stat...)
*/
//If it's the first time we're reading the file (0 bytes read already)
	if(!*bytes_read)
	{
			res = f_stat(file_path,&f_info);
		if(res!= FR_OK) 												//if something went wrong
			{
			SDFile.err = res;										//save error code to the struct
errmsg("f_stat Error: %d\r\n", res);
return res;
			}
			
	res = f_open(&SDFile,file_path,FA_READ);		
			if(res!= FR_OK) 												//if something went wrong
			{
			SDFile.err = res;										//save error code to the struct
errmsg("f_open Error: %d\r\n", res);
return res;
			}	///READ ENTRY 
	res = f_read(&SDFile,NULL,1,NULL);	//to get start of file ptr
			if(res!= FR_OK) 												//if something went wrong
			{
			SDFile.err = res;										//save error code to the struct
				errmsg("f_read Error: %d\r\n", res); 
return res;
			}
		}
	
	/*DO MAGIC */
	
	uint32_t curr_bytes = 0;		//bytes already read in current session
	uint32_t f_start_sect = SDFile.sect;		//save start sector
	uint32_t sect_offset = (SDFile.fptr / _MAX_SS & (SDFatFS.csize - 1));			//sector offset ???
		
	f_cbyte =*bytes_read%_MAX_SS;		//obtain current index of the byte in current sector
	f_clust = SDFile.clust;			//set to start cluster
	f_sect = SDFile.sect;					//set to start of file sector
	f_sect+=*bytes_read/_MAX_SS;			//calculate sector of a file to read (address, NOT current sector of a file!!)	
	
	
	READ_SECTOR:	
	
	f_csect = f_sect - f_start_sect;			//calculate current file sector (starting from beginning of the file, 0th)
		
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
		if(SDFatFS.fs_type == FS_FAT16) fat_sect += f_clust/(_MAX_SS/2);			//calculate FAT sector to read
		else if(SDFatFS.fs_type == FS_FAT32) fat_sect += f_clust/(_MAX_SS/4);	//for FAT32
		if(SD_Driver.disk_read(SDFatFS.drv, sector_buf,fat_sect,1)!=RES_OK)	//read 1 sector of FAT
						{
						res = FR_DISK_ERR;
						SDFile.err = res;										//save error code to the struct
errmsg("SD_Driver.disk_read\r\n"); 
return res;
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
		if(SDFatFS.fs_type == FS_FAT16) fat_sect_idx= f_clust*2 % _MAX_SS;		//calculate index in current FAT sector 
		else if(SDFatFS.fs_type == FS_FAT32)
			{ fat_sect_idx = f_clust*4;
				f_clust %= _MAX_SS;
		  }
		
		f_clust = sector_buf[fat_sect_idx];				//find next cluster of file in FAT table (LOW byte)
		f_clust |= (sector_buf[fat_sect_idx+1])<<8;	//HIGH byte of next cluster. now we have it
		if(SDFatFS.fs_type == FS_FAT32)
		{
			f_clust |= (sector_buf[fat_sect_idx+2])<<16;	//for fat32
			f_clust |= (sector_buf[fat_sect_idx+3])<<24;
			f_clust &= 0x0FFFFFFF;												//for FAT32, only 28 bits are valid for cluster entry
		}
		if(SDFatFS.fs_type == FS_FAT16)
		{
		if(f_clust>=0xFFF8) return FR_NO_FILE;			//end of file or bad cluster
			SDFile.clust = f_clust;
			f_sect = (f_clust-2)*SDFatFS.csize + SDFatFS.database;		//calculate next sector address of file
			f_sect += sect_offset;
		}
		else if (SDFatFS.fs_type == FS_FAT32)
		{
			if(f_clust>=0x0FFFFFF8) return FR_NO_FILE;			//end of file or bad cluster
			SDFile.clust = f_clust;																				//update current cluster in FIL object
			f_sect = (f_clust-2)*SDFatFS.csize + SDFatFS.database;//database???
			f_sect += sect_offset;
		}
		
		else return FR_NO_FILESYSTEM;			//UNKNOWN FS
		
		cl_chg_flg = 0;
	}
	
	if(SD_Driver.disk_read(SDFatFS.drv, sector_buf,f_sect,1)!=RES_OK)	//read 1 sector of a file
	{
						res = FR_DISK_ERR;
						SDFile.err = res;										//save error code to the struct
	errmsg("SD_Driver.disk_read\r\n"); 		
return res;
						}
	
	f_ptr = sector_buf;
	f_ptr +=f_cbyte;		//current position in the temporary buffer
	
	for(uint16_t byte_idx = f_cbyte; byte_idx<_MAX_SS;byte_idx++)		//copy data within one sector
	{
		if(curr_bytes>=bytes_to_bite)	break;				//if we already got all we wanted in current session, break
		if(*bytes_read>=f_info.fsize) 
				{
					goto CLOSE;
				//return EOF; 									//end of file
				}	
		*file_buf++ = sector_buf[byte_idx];						//copy data to user buffer
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
	
	return FR_OK;
	//if we already read the whole file
	CLOSE:		
	res = f_close(&SDFile);				//close file before exit
		if(res!= FR_OK) 												//if something went wrong
			{
			SDFile.err = res;										//save error code to the struct
				errmsg("f_close Error: %d\r\n", res); 
return res;
			}
	return EOF;		//end of file
	
}

/*
Create a blank file or folder
Input: file path
			mode: can be a value of FA_READ, FA_WRITE (doesn't matter for folders)
			autoclose - close file after creating
Return: FRESULT - result of file creation (see ff.h for details)
*/
FRESULT sd_f_create (const char *file_path, uint8_t mode, _Bool autoclose)
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
						if((res!= FR_OK)&&(res!=FR_EXIST)) 												//if something went wrong
							{
								SDFile.err = res;										//save error code to the struct
								errmsg("f_mkdir Error: %d\r\n", res);
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
		errmsg("f_open Error: %d\r\n", res);
		return res;
		}
		
		/*IF AUTOCLOSE*/	
		if(autoclose)
		{
			res =f_close(&SDFile);	//close file after creating
				if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
		errmsg("f_close Error: %d\r\n", res);
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
FRESULT sd_f_close(const char *file_path)
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
			errmsg("f_close Error: %d\r\n", res);
		return res;
		}
		f_cnt--;			//decrement file counter
		//SDFile.obj.lockid = f_cnt;
		//sd_opnd_files[opn_fls].fil_obj = SDFile;
		
#if _USE_LFN
		sd_opnd_files[opn_fls].fil_info.altname[0] = NULL;			//clear LFN altname
#endif
		sd_opnd_files[opn_fls].fil_info.fname[0] = NULL;	//clear file info
		sd_opnd_files[opn_fls].fil_info.fsize = NULL;
	return res;
	}//end if (st_res==SDFS_OPND)
	else return FR_OK;					//otherwise return (no opened file or file not found)
}
/*
Delete a file or a folder
*/
FRESULT sd_f_delete(const char *file_path)
{
	FRESULT res;
	uint8_t opn_fls;
	if(sd_f_stat(file_path,&opn_fls) == SDFS_OPND) 			//if file is opened
	{
		SDFile.err = FR_DENIED;						//return error
		errmsg("sd_f_stat Error: %d\r\n", SDFile.err);
		return FR_DENIED;
	}
	//else
	res = f_unlink(file_path);	//delete file of directory
	if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_unlink Error: %d\r\n", res);
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
SDFSTAT sd_f_stat(const char* path, uint8_t* opnd_file)
{
	//opnd_file = NULL;
	SDFSTAT res;
	char *sop;							//start of path pointer
#if _USE_LFN
#define max_len _MAX_LFN
char lfn_altname[12];			//alternative name in LFN
char *altname_ptr;			//pointer to the array above
f_stat(path,&f_info);	//figure out our file's altname and store it in f_info struct
#else
#define max_len  11	
#endif
	char cpyfilename[max_len];	//array to copy open files' names into
	char cpypathname[max_len];
	uint16_t ccnt=0;	//char counter
	uint16_t len =0;
	sop = (char*)&path[ccnt];		//set the pointer to the beginning of the path
	
	memset(cpyfilename,'\0',sizeof(cpyfilename));
	memset(cpypathname,'\0',sizeof(cpypathname));	//clear name arrays
	memset(lfn_altname,'\0',sizeof(lfn_altname));

	while(path[ccnt]!= '\0')		//search the whole string for any '/'
	{
		if((path[ccnt]=='/') ||(path[ccnt]=='\\'))	{sop = (char*)&path[ccnt]; sop++;} 	//file name starts from here
		ccnt++;		//to the next char
	}
	cpypathname[0] = *sop;
	ccnt=0;

	while(*sop!= '\0')		//'a'->'A'
	{
#if !_USE_LFN									//translate to upper case if LFN is not used
		cpypathname[ccnt] = toupper(*sop++);
#else		
		cpypathname[ccnt] = *sop++;	//copy path if LFN used
#endif
		ccnt++;		//to the next char
	}//end while
	
#if _USE_LFN	
	len = strlen(cpypathname);		//calculate the length of the file name from the input path
	if(len<12)
	{
		ccnt=0;
	while(len)							//convert filename to uppercase if it's an SFN
	{
		cpypathname[ccnt] = toupper(cpypathname[ccnt]);
		ccnt++;
		len--;
	}
	}//end if len<12
#endif	
	//Now when we've found the start of filename, we can compare it to the previously opened ones
	
	for(uint8_t i=0;i<_FS_LOCK;i++)
	{
		//look if we have the file already open
		
		char *fnameptr;		//pointer of the begin of the filename

		fnameptr = sd_opnd_files[i].fil_info.fname;		//set the pointer at the beginning of the (potentionally) open file name																							
		
		char *cpyfnameptr = cpyfilename;				//set the pointer at the beginning of the file name copy
		
		while(*fnameptr!='\0')		//until the end of the filename
		{
			*cpyfnameptr++ = *fnameptr++;		//copy the name of the opened file into our cpy array
		}//end while
		
		len = strlen(cpypathname);		//calculate the length of the file name from the input path
		if(!(strncmp(cpyfilename,cpypathname,len)))	//check if the file has been opened already
			{
			res = SDFS_OPND; //if it has, semaphore that
			*opnd_file = i;		//obtain the pointer to the existing file
			return res;}		//if we found that such file is already open, return
		
#if _USE_LFN
		
		sop = sd_opnd_files[i].fil_info.altname;			//I just use this *sop pointer here as it does nothing at this point anyway 
																									//(now it points at the alternative name for LFN)
		altname_ptr = lfn_altname;									//point at the beginning of our temporary lfn_altname array	
		if(*sop != '\0')
		{
			while(*sop != '\0')	//attention - in short files' altnames first char can be '\0'
			{
			*altname_ptr++ = *sop++;				//copy existing (if it exists at all) alt name to our array
			}
			
			if(!(strncmp(lfn_altname,f_info.altname,12)))	//check if the file has been opened already
			{
			res = SDFS_OPND; //if it has, semaphore that
			*opnd_file = i;		//obtain the pointer to the existing file
			return res;}		//if we found that such file is already open, return
		}
#endif
			
	}//end for
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
			errmsg("f_mkfs error: %d\r\n", SDFile.err);
			return res;
		}
	res =	f_mount(&SDFatFS,SDPath,SD_MOUNT_NOW);		//mount disk
		if(res!= FR_OK) 												//if something went wrong
		{
			SDFile.err = res;										//save error code to the struct
			errmsg("f_mount error: %d\r\n", SDFile.err);
			return res;
		}
	return res;
}

//
FRESULT sd_f_get_size(const char *file_path, uint32_t *fsize)
{
	FILINFO f_info;
	FRESULT res;
	memset(&f_info, 0, sizeof(FILINFO));
	
	if ((res = f_stat(file_path, &f_info)) != FR_OK)
	{
		errmsg("f_stat() ERROR: %d\r\n", res);
		*fsize = 0;
	}
	else *fsize = f_info.fsize;
	
	return res;
}





