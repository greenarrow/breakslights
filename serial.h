#ifndef SERIAL_H
#define SERIAL_H

#ifndef POSIX
#include <Arduino.h>
#else
#include <stdbool.h>
#endif

void serial_init(long rate);
void serial_println(const char *line);
byte serial_getbyte();
void serprint(const char *fmt, ...);

#endif
