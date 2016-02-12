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

byte serial_getbyte()
{
	if (Serial.available() == 0)
		return 0;

	return Serial.read();
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
