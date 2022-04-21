/**
  ******************************************************************************
  * @file           : dwitch.h
  * @brief          : Header of Module reading CNCU DIPSWITCHs
  ******************************************************************************
*/

#ifndef _CNCU_DSWITCH_H
  #define _CNCU_DSWITCH_H

/* Includes ------------------------------------------------------------------*/
  #include "stm32f4xx_hal.h"
 
/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/ 
/**
* @brief 	Initialize GPIO for reading onboard  DIP switch and get it's value   
* @param	None
* @retval	8-bit value from SIP switches
*/
  uint8_t Get_DSwitch (void);
  
#endif // DSWITCH
