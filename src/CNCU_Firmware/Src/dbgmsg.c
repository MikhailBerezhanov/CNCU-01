#define DEBUG_MSG 1
#include <dbgmsg.h>

#define msg __dbgmsg

#define HEXLINE_MOD 16
char __dbgmsgstr[__dbgmsgstr_len];

static unsigned char
toprint (const unsigned char chr) {
	// https://www.ascii-codes.com/cp855.html
	// Standard (32 - 126) and extended (128 - 254) character set :
	return ((chr > 0x1f && chr < 0x7f) || (chr >= 0x80 && chr < 0xFF)) ? chr : '.';
} /* toprint() */

static void
hexline (const unsigned char *dump, size_t len) {
	size_t i;
	char str[HEXLINE_MOD + 1] = {0};

	for (i = 0; i < len && i < HEXLINE_MOD; i++) {
		msg ("%02X ", dump[i]);
		str[i] = toprint (dump[i]);
		if (i == HEXLINE_MOD / 2 - 1)
			msg (" ");
	}
	for (; i < HEXLINE_MOD; i++) {
		if (i == HEXLINE_MOD / 2 - 1)
			msg (" ");
		msg ("   ");
	}
	msg (" |%s|", str);
} /* hexline() */

void
hexdump (const void *_dump, size_t len, size_t offset) {
	const unsigned char *dump = (const unsigned char*) _dump;
	for (size_t i = 0; i < len; i += HEXLINE_MOD) {

		msg ("%08zx  ", offset + i);

		hexline (&dump[i], len - i);
		msg ("\n");
	}
} /* hexdump() */

void
hexstr (const void *_dump, size_t len) {
	const unsigned char *dump = (unsigned char*) _dump;
	size_t i;
	for (i = 0; i < len; i++) {
		if (i != 0 && i % (HEXLINE_MOD / 2) == 0)
			msg ("  ");
		if (i != 0 && i % HEXLINE_MOD == 0)
			msg ("\n");
		msg ("%02X ", dump[i]);
	}
	msg ("\n");
} /* hexstr() */

// Print array data
void print_arr(char *msg, unsigned char *buf, int buflen)
{
    dbgmsg("%s", msg);
    for (int i = 0; i < buflen; i++)
        dbgmsg("%02X ", buf[i]);
    dbgmsg("\r\n");
}


