/******************************************************************************/
/* RETARGET.C: 'Retarget' layer for target-dependent low level functions      */
/******************************************************************************/
/* This file is part of the uVision/ARM development tools.                    */
/* Copyright (c) 2005 Keil Software. All rights reserved.                     */
/* This software may only be used under the terms of a valid, current,        */
/* end user licence from KEIL for a compatible version of KEIL software       */
/* development tools. Nothing else gives you the right to use this software.  */
/******************************************************************************/

#include <stdio.h>
#include <rt_misc.h>
#include "uart.h"
#include "main.h"
//#pragma import(__use_no_semihosting_swi)

struct __FILE { int handle; /* Add whatever you need here */ };
FILE __stdout;
FILE __stdin;

int fputc(int ch, FILE *f) 
{
#if (DEBUG_PORT_USB)
	USART1_Send((uint8_t *)&ch, 1);
#endif
#if (DEBUG_PORT_UART)
	USART3_Send((uint8_t *)&ch, 1);
#endif
	return ch;
}

int fgetc(FILE *f) 
{
    uint8_t ch = 0;
#if (DEBUG_PORT_USB)
    if (!USART1_Get(&ch, 1)) return ch;
#endif
#if (DEBUG_PORT_UART)	
		if (!USART3_Get(&ch, 1)) return ch;
#endif
    return ch;
}

int ferror(FILE *f) {
  /* Your implementation of ferror */
  return EOF;
}


void _ttywrch(int ch) 
{
//  sendchar (ch);
}


void _sys_exit(int return_code) 
{
  while (1);    /* endless loop */
}
