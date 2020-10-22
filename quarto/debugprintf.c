#include "debug/printf.h"

#ifdef PRINT_DEBUG_STUFF

#include "avr/pgmspace.h"
#include <stdarg.h>
#include "imxrt.h"

void putchar_debug(char c);
static void puint_debug(unsigned int num);

#define printf_debug printf

static void puint_debug(unsigned int num)
{
	char buf[12];
	unsigned int i = sizeof(buf)-2;

	buf[sizeof(buf)-1] = 0;
	while (1) {
		buf[i] = (num % 10) + '0';
		num /= 10;
		if (num == 0) break;
		i--;
	}
	printf_debug(buf + i);
}

#define putchar_debug putchar


FLASHMEM void printf_debug_init(void){};


#endif // PRINT_DEBUG_STUFF
