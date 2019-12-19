

#ifndef __SDCARD_H
#define __SDCARD_H

#include "stm32f4xx_hal.h"
#include "ff.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h" 				/* defines SD_Driver as external */
#include "cncu_conf.h"

#define F_AUTOCLOSE_ON 1			//autoclose on/off for file read/write operations
#define F_AUTOCLOSE_OFF 0		

#define SD_MOUNT_NOW	1				//used in f_mount function
#define SD_MOUNT_LATER	0

/*SD File Status (use in sd_f_stat() function*/
typedef enum {
	SDFS_NOPNYT = 0,						/* (0) File exists and not open yet */
	SDFS_OPND,									/*File already opened*/
	SDFS_NOFL,									/*No such file or directory*/
} SDFSTAT;

typedef struct
{
	FILINFO fil_info;				//file info
	FIL fil_obj;						//file struct 
}OFLIST;			//list of opened files

#if 1
extern char SDPath[4]; 					/* SD logical drive path */
extern FATFS SDFatFS; 					/* File system object for SD logical drive */
extern FIL SDFile; 							/* File object for SD */
#endif

uint8_t SD_DMA_Init(void);
void SD_DMA_Reinit(void);

/*FAT files errors (see ff.h file -> FRESULT)*/	
//	FR_OK = 0,							/* (0) Succeeded */
//	FR_DISK_ERR,						/* (1) A hard error occurred in the low level disk I/O layer */
//	FR_INT_ERR,							/* (2) Assertion failed */
//	FR_NOT_READY,						/* (3) The physical drive cannot work */
//	FR_NO_FILE,							/* (4) Could not find the file */
//	FR_NO_PATH,							/* (5) Could not find the path */
//	FR_INVALID_NAME,				/* (6) The path name format is invalid */
//	FR_DENIED,							/* (7) Access denied due to prohibited access or directory full */
//	FR_EXIST,								/* (8) Access denied due to prohibited access */
//	FR_INVALID_OBJECT,			/* (9) The file/directory object is invalid */
//	FR_WRITE_PROTECTED,			/* (10) The physical drive is write protected */
//	FR_INVALID_DRIVE,				/* (11) The logical drive number is invalid */
//	FR_NOT_ENABLED,					/* (12) The volume has no work area */
//	FR_NO_FILESYSTEM,				/* (13) There is no valid FAT volume */
//	FR_MKFS_ABORTED,				/* (14) The f_mkfs() aborted due to any problem */
//	FR_TIMEOUT,							/* (15) Could not get a grant to access the volume within defined period */
//	FR_LOCKED,							/* (16) The operation is rejected according to the file sharing policy */
//	FR_NOT_ENOUGH_CORE,			/* (17) LFN working buffer could not be allocated */
//	FR_TOO_MANY_OPEN_FILES,	/* (18) Number of open files > _FS_LOCK */
//	FR_INVALID_PARAMETER		/* (19) Given parameter is invalid */	

FRESULT sd_mount(void);
FRESULT sd_unmount(void);
FRESULT sd_remount(void);

FRESULT sd_f_read_(const char* fname, void *buf, size_t buf_size, _Bool unused);

void sd_get_info(uint8_t *data_buf);						//get sd card specs
uint32_t sd_n_files(void);	//return total number of files on disk
FRESULT sd_f_get_size(const char *file_path, uint32_t *fsize);
FRESULT sd_dir_files_list (const char *dir_path, uint8_t *data_buf, uint32_t buf_size, uint32_t *file_cnt);		//list of files in folder (dir_path = NULL - search in ROOT)
FRESULT sd_format(uint8_t FAT_type, uint8_t clu_size, uint8_t *f_buf, uint32_t f_buf_size);	//format SD card. This action will erase all the data!
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

f_buf - buffer to format the card (the bigger - the better and faster)
f_buf_size - size of buffer (better use buffer of at least 1 sector (512 bytes default, otherwise - _MAX_SS))
*/
int sd_f_seq_read (const char *file_path, void *file_buf, uint32_t bytes_to_bite,uint32_t *bytes_read);	//sequential read (for large files). returns FRESULT or EOF (-1)
FRESULT sd_f_read(const char *file_path, void *file_buf, uint32_t bytes_to_read, _Bool autoclose);								//open and read a file to a buffer
FRESULT sd_f_write(const char *file_path, void *wr_buf,uint32_t bytes_to_write, _Bool autoclose);				//open file and WRITE DATA to it
FRESULT sd_f_ovrt(const char *file_path, void *wr_buf,uint32_t bytes_to_write, _Bool autoclose);				//open and overwrite file with DATA
FRESULT sd_f_swr(const char *file_path, void *wr_buf, _Bool autoclose);					//write STRING to file
FRESULT sd_f_sovr(const char *file_path, void *wr_buf, _Bool autoclose);			//overwrite file with STRING
FRESULT sd_f_close(const char *file_path);								//close previously opened file
FRESULT sd_f_create(const char *file_path, uint8_t mode, _Bool autoclose);		//create a new file (mode - FA_READ, FA_WRITE, see ff.h)
FRESULT sd_f_delete(const char *file_path);								//delete a file or folder (if opened or directory is not empty, res = FA_DENIED)
SDFSTAT sd_f_stat(const char* path,uint8_t* opnd_file);	//obtain file status (open/ not open/ not exist)

extern SD_HandleTypeDef hsd;
extern DMA_HandleTypeDef hdma_sdio_rx;
extern DMA_HandleTypeDef hdma_sdio_tx;

#endif
