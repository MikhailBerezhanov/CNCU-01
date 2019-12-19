/*
Source for RTC module functions (STM32F407VET6)
Based on HAL driver
*/
#include "rtc.h"

#define DEBUG_MSG DEBUG_RTC
#include "dbgmsg.h"

static RTC_HandleTypeDef hrtc; //RTC config info struct

/**
* @brief RTC MSP Initialization
* This function configures the hardware resources used in this example
* @param hrtc: RTC handle pointer
* @retval None
*/
void HAL_RTC_MspInit(RTC_HandleTypeDef* hrtc)
{
  if(hrtc->Instance==RTC)
  {
    /* Peripheral clock enable */
    __HAL_RCC_RTC_ENABLE();
  }
}

/**
* @brief RTC MSP De-Initialization
* This function freeze the hardware resources used in this example
* @param hrtc: RTC handle pointer
* @retval None
*/
void HAL_RTC_MspDeInit(RTC_HandleTypeDef* hrtc)
{
  if(hrtc->Instance==RTC)
  {
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();
  }
}

uint8_t RTC_Init(void)
{
	/*Initialize RTC and LSE/LSI (if not initialized yet) */
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;
  clk_source_t src = LSE_TCLK;  

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
	PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;		
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)			// LSE init failed
	{
		errmsg("-LSE Init failed, Trying to init LSI\r\n");

		RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI;
		RCC_OscInitStruct.LSEState = RCC_LSI_ON;
		PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
		src = LSI_TCLK;
		if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)		// LSI init failed
		{
			errmsg("Failed to init TCLK source\r\n");
			return INIT_ERR;
		}
	}	

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)		
  {
    errmsg("Failed to Init PeriphClk\r\n");
		return INIT_ERR;
	}

	dbgmsg("-RTC TCLK source:\t%s\r\n", src ? "LSI" : "LSE");
	
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)								
  {
    errmsg("RTC Init Failed\r\n");
		return INIT_ERR;
  }
	
#ifdef RTC_SET_COMPILE_DATE			//defined in stm32_rtc.h
	RTC_Set_CompileTime();	
#endif
	return CNCU_OK;
}

/*
	RTC_Get_Time
	Get current RTC time values and write them into sTime struct
*Input: RTC_TimeTypeDef *sTime - struct to store RTC time in
*Output: None
*/
void RTC_Get_Time(RTC_TimeTypeDef *sTime)
{
	uint8_t hal_status = HAL_RTC_GetTime(&hrtc,sTime,RTC_FORMAT_BIN);	//get RTC time in binary format
	if(hal_status != HAL_OK)
	{
		errmsg("RTC_Get_Time Error\r\n");			
	}
}

/*
	RTC_Get_Date (Must be called after RTC_Get_Time())
	Get current RTC date and write into sDate struct
	The YEAR is the number of how many years passed since 1980, so.. you got it
*Input: RTC_DateTypeDef *sDate - struct to store RTC date in
*Output: None
*/
void RTC_Get_Date(RTC_DateTypeDef *sDate)
{
	uint8_t hal_status = HAL_RTC_GetDate(&hrtc,sDate,RTC_FORMAT_BIN);	//get RTC date in binary format
	if(hal_status != HAL_OK)
	{
		errmsg("RTC_Get_Date Error\r\n");			
	}
}

/*
	RTC_Set_Time
	Set RTC time from sTime struct
*Input: RTC_DateTypeDef *sTime - struct to read RTC time from
*Output: None
*/
void RTC_Set_Time(uint8_t hour, uint8_t minute, uint8_t second)
{
	RTC_TimeTypeDef sTime;
	sTime.Hours = hour;
	sTime.Minutes = minute;
	sTime.Seconds = second;
	sTime.SubSeconds = 0;
	
	uint8_t hal_status = HAL_RTC_SetTime(&hrtc,&sTime,RTC_FORMAT_BIN);	//set RTC time in binary format
	if(hal_status != HAL_OK)
	{
		errmsg("RTC_Set_Time Error\r\n");			
	}
}
/*
	RTC_Set_Date 
	Set current RTC date from sDate struct
*Input: RTC_DateTypeDef *sDate - struct to read RTC date from
*Output: None
*/
void RTC_Set_Date(uint8_t day, uint8_t weekday, uint8_t month, uint16_t year)
{
	RTC_DateTypeDef sDate;
	sDate.Date = day;
	sDate.WeekDay = weekday;
	sDate.Month = month;
	sDate.Year = (year - 1980);		//calculate the year since 1980
	
	uint8_t hal_status = HAL_RTC_SetDate(&hrtc,&sDate,RTC_FORMAT_BIN);	//set RTC date in binary format
	if(hal_status != HAL_OK)
	{
		errmsg("RTC_Set_Date Error\r\n");
	}
}
//retrieve compilation date and time from __DATE__, __TIME__ macroses
void RTC_Set_CompileTime(void)
{
	RTC_DateTypeDef compile_date;
	RTC_TimeTypeDef compile_time;
	
	char rtc_buf[4];		//string temporary buffer to keep __DATE__, __TIME__ strings and retrieve actual values from them
	uint8_t s_cnt=0;			//string characters' counter
	/*Get DATE*/
	while(__DATE__[s_cnt]!= ' ')	//search for the month
	{
		rtc_buf[s_cnt] = __DATE__[s_cnt];	//now the month is in 'Jan' format in our buffer
		s_cnt++;
	}
	s_cnt++;		//get past ' '
	//Assign rtc month to the corresponding month
	if(!(strncmp((char*)rtc_buf,"Jan",3)))
	{
		compile_date.Month = RTC_MONTH_JANUARY;
	}
	else if(!(strncmp((char*)rtc_buf,"Feb",3)))
	{
		compile_date.Month = RTC_MONTH_FEBRUARY;
	}
	else if(!(strncmp((char*)rtc_buf,"Mar",3)))
	{
		compile_date.Month = RTC_MONTH_MARCH;
	}
	else if(!(strncmp((char*)rtc_buf,"Apr",3)))
	{
		compile_date.Month = RTC_MONTH_APRIL;
	}
	else if(!(strncmp((char*)rtc_buf,"May",3)))
	{
		compile_date.Month = RTC_MONTH_MAY;
	}
	else if(!(strncmp((char*)rtc_buf,"Jun",3)))
	{
		compile_date.Month = RTC_MONTH_JUNE;
	}
	else if(!(strncmp((char*)rtc_buf,"Jul",3)))
	{
		compile_date.Month = RTC_MONTH_JULY;
	}
	else if(!(strncmp((char*)rtc_buf,"Aug",3)))
	{
		compile_date.Month = RTC_MONTH_AUGUST;
	}
	else if(!(strncmp((char*)rtc_buf,"Sep",3)))
	{
		compile_date.Month = RTC_MONTH_SEPTEMBER;
	}
	else if(!(strncmp((char*)rtc_buf,"Oct",3)))
	{
		compile_date.Month = RTC_MONTH_OCTOBER;
	}
	else if(!(strncmp((char*)rtc_buf,"Nov",3)))
	{
		compile_date.Month = RTC_MONTH_NOVEMBER;
	}
	else if(!(strncmp((char*)rtc_buf,"Dec",3)))
	{
		compile_date.Month = RTC_MONTH_DECEMBER;
	}
	
	while(__DATE__[s_cnt]!= ' ')	//search for the Day
	{
		rtc_buf[s_cnt-4] = __DATE__[s_cnt];	//
		s_cnt++;
	}
	s_cnt++;	//get past ' '
	
	int i;
	sscanf(rtc_buf,"%d",&i);
	compile_date.Date = i;		//set the Day
	
	while(__DATE__[s_cnt]!= '\0')	//search for the Year
	{
		rtc_buf[s_cnt-7] = __DATE__[s_cnt];	//
		s_cnt++;
	}
	sscanf(rtc_buf,"%d",&i);
	compile_date.Year = (i-1980);		//Set the Year
	compile_date.WeekDay = 1; 			//Set weekday as Monday, so it won't affect the year
	/*Get TIME*/
	rtc_buf[0] = '\0';
	rtc_buf[1] = '\0';
	rtc_buf[2] = '\0';
	rtc_buf[3] = '\0';
	
	s_cnt=0;			//clear counter
	while(__TIME__[s_cnt]!= ':')	//search for the hour
	{
		rtc_buf[s_cnt] = __TIME__[s_cnt];	
		s_cnt++;
	}
	s_cnt++;		//get past ':'
	sscanf(rtc_buf,"%d",&i);
	compile_time.Hours = i;		//Set the Hour
	
	while(__TIME__[s_cnt]!= ':')	//search for the minutes
	{
		rtc_buf[s_cnt-3] = __TIME__[s_cnt];	
		s_cnt++;
	}
	s_cnt++;		//get past ':'
	sscanf(rtc_buf,"%d",&i);
	compile_time.Minutes = i;		//Set the Minutes
	
	while(__TIME__[s_cnt]!= '\0')	//search for the minutes
	{
		rtc_buf[s_cnt-6] = __TIME__[s_cnt];	
		s_cnt++;
	}
	sscanf(rtc_buf,"%d",&i);
	compile_time.Seconds = i;		//Set the Seconds
	
	uint8_t hal_status = HAL_RTC_SetDate(&hrtc,&compile_date,RTC_FORMAT_BIN);	//set RTC date in binary format
	if(hal_status != HAL_OK)
	{
		errmsg("RTC Error\r\n");
	}		
	
	hal_status = HAL_RTC_SetTime(&hrtc,&compile_time,RTC_FORMAT_BIN);	//set RTC time in binary format
	if(hal_status != HAL_OK)
	{
		errmsg("RTC Error\r\n");			
	}
}
