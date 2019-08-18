#ifndef __dbgmsg_h
#define __dbgmsg_h

#include <stdio.h>
#include <string.h>
#include <errno.h>

void hexstr (const void *dump, size_t len);
void hexdump (const void *dump, size_t len, size_t offset);
void print_arr(char *msg, unsigned char *buf, int buflen);

#define __dbgmsgstr_len 128
extern char __dbgmsgstr[__dbgmsgstr_len];

// dont use this macros directly!
#define __dbgmsg(__dbgarg...) do { \
	printf (__dbgarg); \
	fflush (stdout); \
} while (0)

#if DEBUG_MSG
  #define dbgmsg __dbgmsg
  #define dbghex(dump, len) hexstr(dump, len)
	#define dbghd(dump, len) hexdump(dump, len, 0)
	#define dbghd_o(dump, len, off) hexdump(dump, len, off)
#else /* DEBUG_MSG */
	#define dbgmsg(arg...)
	#define dbghex(dump, len)
	#define dbghd(dump, len)
	#define dbghd_o(dump, len, off)
#endif /* DEBUG_MSG */

#define _RED     "\x1b[31m"
#define _GREEN   "\x1b[32m"
#define _YELLOW  "\x1b[33m"
#define _BLUE    "\x1b[34m"
#define _MAGENTA "\x1b[35m"
#define _CYAN    "\x1b[36m"
#define _LGRAY   "\x1b[90m"
#define _RESET   "\x1b[0m"
#define _C(e) e?_RED:_GREEN

#define resmsg(assertion, args...) do \
{ \
	snprintf(__dbgmsgstr, __dbgmsgstr_len, ## args); \
	if (!(assertion)) \
	{ \
	snprintf(&__dbgmsgstr[strlen(__dbgmsgstr)], __dbgmsgstr_len - strlen(__dbgmsgstr), _GREEN"OK\r\n"_RESET); \
	} \
	else \
	{ \
	snprintf(&__dbgmsgstr[strlen(__dbgmsgstr)], __dbgmsgstr_len - strlen(__dbgmsgstr), _RED"FAIL\r\n"_RESET); \
	} \
	__dbgmsg (__dbgmsgstr); \
} while(0);

#define errmsg(args...)  do \
{ \
	snprintf(__dbgmsgstr, __dbgmsgstr_len, _RED"[%s()] error: "_RESET, __func__);\
	snprintf(&__dbgmsgstr[strlen(__dbgmsgstr)], __dbgmsgstr_len - strlen(__dbgmsgstr), ## args);\
	if (__dbgmsgstr[strlen (__dbgmsgstr)-1] != '\n') \
	{ \
		__dbgmsgstr[strlen (__dbgmsgstr)+1] = 0; \
		__dbgmsgstr[strlen (__dbgmsgstr)] = '\n'; \
	} \
	__dbgmsg (__dbgmsgstr); \
} while(0);

#define perrmsg(args...) do \
{ \
	snprintf (__dbgmsgstr, __dbgmsgstr_len, "\t%s[%s()]%s (%d)", errno?_RED:_GREEN, __func__, _RESET, errno); \
	snprintf (&__dbgmsgstr[strlen(__dbgmsgstr)], __dbgmsgstr_len - strlen(__dbgmsgstr), ## args); \
	perror(__dbgmsgstr); \
} while(0);

#endif
