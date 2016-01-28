/*
 * C wrapper around Arduino Serial class.
 */

#include <Arduino.h>

extern "C"
{

#include "serial.h"

void serial_init(long rate)
{
	Serial.begin(rate);
}

void serial_println(const char *line)
{
	Serial.println(line);
}

byte serial_getdelim(const char delim, char *buf, size_t len)
{
	byte nb = 0;
	int b;

	for (;;) {
		if (nb >= len)
			return 0;

		if (Serial.available() == 0) {
			delay(1);
			continue;
		}

		b = Serial.read();

		if (b == -1)
			return 0;

		buf[nb] = b;
		nb++;

		if (b == delim) {
			buf[nb] = '\0';
			return nb;
		}
	}

	return nb;
}

void serprint(const char *fmt, ...)
{
	char line[256];
	va_list args;

	va_start(args, fmt);
	vsnprintf(line, 255, fmt, args);
	va_end(args);

	serial_println(line);
}

}
