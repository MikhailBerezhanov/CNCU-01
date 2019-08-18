/*
Header file for SD CARD operations, stm32f407vet
Based on HAL

Files that affects SD+FAT:
bsp_driver_sd c/h
diskio c/h
	stm32_rtc c/h
sd_diskio c/h
fatfs c/h
ff c/h
ffconf h
ff_gen_drv c/h


*/
#ifndef __SDCARD_H_
#define __SDCARD_H_

//#pragma ONCE

#ifndef HAL_SD_MODULE_ENABLED
 #define HAL_SD_MODULE_ENABLED
#endif /* HAL_SD_MODULE_ENABLED */

#define SD_ERROR_REPORT							//comment/uncomment to allow/disallow SD-card Error report in _Error_Handler()

#define SD_MOUNT_NOW	1				//used in f_mount function
#define SD_MOUNT_LATER	0

#define F_AUTOCLOSE_ON 1		//autoclose on/off for file read/write operations
#define F_AUTOCLOSE_OFF 0

#include "stm32f4xx_hal.h"
#include "fatfs.h"					//contains basic functions (f_open, f_read, etc..)
#include "string.h"
#include "main.h"

//Prorotypes
/*SD File Status (use in sd_f_stat() function*/
typedef enum {
	SDFS_NOPNYT = 0,				/* (0) File exists and not open yet */
	SDFS_OPND,						/*File already opened*/
	SDFS_NOFL,						/*No such file or directory*/
} SDFSTAT;

typedef struct
{
	FILINFO fil_info;		//file info
	FIL fil_obj;					//file struct 
}OFLIST;			//list of opened files

/*
SEE BASIC FUNCTIONS IN ff.h/ff.c files (but use at your own  risk)
*/
uint32_t SD_CARD_Init(void);	//SD card and FAT system init.

void sd_print_info(void);						//get sd card specs
uint32_t sd_n_files(const Diskio_drvTypeDef *SD_Drv, FATFS *FAT_struct);	//return total number of files on disk (see fatfs.c)

/*TO BE TESTED!!!!*/
FRESULT sd_dir_files_list (char *dir_path, uint8_t *data_buf, uint32_t buf_size);		//list of files in folder (dir_path = NULL - search in ROOT)	
/*TO BE TESTED!!!!*/


FRESULT sd_f_read(char *file_path,void *file_buf, uint32_t bytes_to_read, _Bool autoclose);								//open and read a file to a buffer
FRESULT sd_f_write(char *file_path,void *wr_buf,uint32_t bytes_to_write, _Bool autoclose);				//open file and WRITE DATA to it
FRESULT sd_f_ovrt(char *file_path,void *wr_buf,uint32_t bytes_to_write, _Bool autoclose);				//open and overwrite file with DATA
FRESULT sd_f_swr(char *file_path,void *wr_buf, _Bool autoclose);					//write STRING to file
FRESULT sd_f_sovr(char *file_path,void *wr_buf, _Bool autoclose);			//overwrite file with STRING
FRESULT sd_f_close(char *file_path);								//close previously opened file
FRESULT sd_f_create (char *file_path, uint8_t mode, _Bool autoclose);		//create a new file (mode - FA_READ, FA_WRITE)

FRESULT sd_f_delete(char *file_path);								//delete a file or folder (if opened, res = FA_DENIED)

SDFSTAT sd_f_stat(char* path,uint8_t* opnd_file);	//obtain file status (open/ not open/ not exist)

FRESULT sd_format(uint8_t FAT_type, uint8_t clu_size, uint8_t *f_buf, uint32_t f_buf_size);

SDFSTAT add_http_header(char *file_path,uint8_t *filedata, uint8_t *out_data, uint32_t* out_len);	//HTTP HEADER....

//Externs
extern SD_HandleTypeDef hsd;	//sd card structure object (sdcard.c)
extern FIL sd_file[_FS_LOCK];       //File struct array to store multiple simultaneously opened files (number of files determined by _FS_LOCK in ffconf.h)
extern FILINFO sd_file_info[_FS_LOCK];
extern uint8_t f_cnt;			//opened files counter 
//from fatfs.c:
extern uint8_t retSD;    /* Return value for SD */
extern char SDPath[4];   /* SD logical drive path */
extern FATFS SDFatFS;    /* File system object for SD logical drive (stores FAT info: type, number of drives, clusters, etc)*/		
extern FIL SDFile;       /* File object for SD */
extern FILINFO f_info;					//file information (size, name, date, etc) (declared in fatfs.c)

#endif
