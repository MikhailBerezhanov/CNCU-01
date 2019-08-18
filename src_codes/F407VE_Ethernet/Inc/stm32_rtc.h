/*
Header file for RTC module functions (STM32F407VET6)
Based on HAL driver
*/

#ifndef __STM32_RTC_H
#define __STM32_RTC_H

#include "main.h"
#include <stdint.h>
#include <string.h>

#if defined STM32F407xx
#include "stm32f4xx_hal.h"
#else 
	#warning "RTC functions in stm32_rtc.c file are available only for STM32F407xx yet"
#endif
	
//#define RTC_SET_COMPILE_DATE		//comment this to disallow compile date/time setting after reset/powerup (see RTC_Init())

typedef enum
{
	LSE_TCLK = 0,
	LSI_TCLK
}clk_source_t;

typedef enum
{
	RTC_OK = 0,
	TCLK_ERROR,
	RTC_INIT_ERROR,
	SET_ERROR,
	GET_ERROR
}rtc_err_t;

//extern vars
 uint8_t RTC_Init(void);
 
 void RTC_Get_Time(RTC_TimeTypeDef *sTime);
 void RTC_Get_Date(RTC_DateTypeDef *sDate);		//retrieve date from RTC (year is biased off 1980th)
 
 void RTC_Set_Time(uint8_t hour, uint8_t minute, uint8_t second);
 void RTC_Set_Date(uint8_t day, uint8_t weekday, uint8_t month, uint16_t year);
 
 void RTC_Set_CompileTime(void);	//set the compile time to RTC
#endif
