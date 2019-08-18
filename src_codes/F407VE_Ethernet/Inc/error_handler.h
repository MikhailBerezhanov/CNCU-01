
/*
Error Handler header file
*/

#include "sdcard.h"

#define DEBUG	1

#if DEBUG
#define DEBUG_PORT_USB	1
#define dbgprint printf
#else
#define dbgprint(arg...)
#endif
#ifndef Error_Handler
void _Error_Handler(char *file, int line);
#endif