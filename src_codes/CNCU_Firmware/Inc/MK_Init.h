
/*==============================================================================
>File name:  	Init.h
>Brief:         All initialize functions must be here
                
>Author:        LL
>Date:          23.09.2018
>Version:       1.0
==============================================================================*/

#ifndef _MK_INIT_H
#define _MK_INIT_H

#include <stdint.h>

char MK_SysClock_Config(void);
void MK_GPIO_Init(void);
char MK_IWDG_Init(uint32_t reload);
void MK_IWDG_Reset(void);

#endif /* _MK_INIT_H */
