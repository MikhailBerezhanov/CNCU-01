
/*
ERROR HANDLER
*/
#include "error_handler.h"

#if DEBUG	//if debug is allowed 

void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
#ifdef SD_ERROR_REPORT 			//if sd-card error report is allowed
#ifdef __stdio_h						//if stdio.h is included
	/*SD-card errors (see full list in stm32f4xx_hal_sd.h -> SDMMC_ERROR_NONE))*/
	if(hsd.ErrorCode != HAL_SD_ERROR_NONE)
	{
		
		dbgprint("SD ERROR:%#01x\r\nFILE:%s\r\nLINE:%d\r\n",hsd.ErrorCode,file,line);

		return;
	}
	/*FAT files errors (see ff.h file -> FRESULT)*/	
//FR_OK = 0,				/* (0) Succeeded */
//	FR_DISK_ERR,			/* (1) A hard error occurred in the low level disk I/O layer */
//	FR_INT_ERR,				/* (2) Assertion failed */
//	FR_NOT_READY,			/* (3) The physical drive cannot work */
//	FR_NO_FILE,				/* (4) Could not find the file */
//	FR_NO_PATH,				/* (5) Could not find the path */
//	FR_INVALID_NAME,		/* (6) The path name format is invalid */
//	FR_DENIED,				/* (7) Access denied due to prohibited access or directory full */
//	FR_EXIST,				/* (8) Access denied due to prohibited access */
//	FR_INVALID_OBJECT,		/* (9) The file/directory object is invalid */
//	FR_WRITE_PROTECTED,		/* (10) The physical drive is write protected */
//	FR_INVALID_DRIVE,		/* (11) The logical drive number is invalid */
//	FR_NOT_ENABLED,			/* (12) The volume has no work area */
//	FR_NO_FILESYSTEM,		/* (13) There is no valid FAT volume */
//	FR_MKFS_ABORTED,		/* (14) The f_mkfs() aborted due to any problem */
//	FR_TIMEOUT,				/* (15) Could not get a grant to access the volume within defined period */
//	FR_LOCKED,				/* (16) The operation is rejected according to the file sharing policy */
//	FR_NOT_ENOUGH_CORE,		/* (17) LFN working buffer could not be allocated */
//	FR_TOO_MANY_OPEN_FILES,	/* (18) Number of open files > _FS_LOCK */
//	FR_INVALID_PARAMETER	/* (19) Given parameter is invalid */	

	if(SDFile.err != FR_OK)
	{
		dbgprint("FILE ERROR:%d\r\nFILE:%s\r\nLINE:%d\r\n",SDFile.err,file,line);

		return;
	}
	
	#else 
	#warning "_Error_Handler: stdio.h not included, SD Error report is bypassed"
#endif
	#else 
	#warning "_Error_Handler: SD_ERROR_REPORT is undefined, SD Error report is bypassed"
#endif
	
  /* USER CODE END Error_Handler_Debug */
}
#endif