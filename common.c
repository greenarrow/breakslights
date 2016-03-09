#include <stdio.h>

#include "uart.h"
#include "common.h"

void serprint(const char *fmt, ...)
{
	char line[256];
	va_list args;

	va_start(args, fmt);
	vsnprintf(line, 255, fmt, args);
	va_end(args);

	uart_puts(line);
	uart_puts("\r\n");
}
