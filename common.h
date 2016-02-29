#ifndef COMMON_H
#define COMMON_H

#ifdef POSIX
#include <stdio.h>
#else
#include "serial.h"
#endif

#include <stdarg.h>

#ifdef POSIX
#define error(...) { \
	fprintf(stderr, "%s:%d: ", __FILE__, __LINE__); \
	fprintf(stderr, __VA_ARGS__); \
	fputc('\n', stderr); \
}
#define output(...) { \
	fprintf(stdout, "%s:%d: ", __FILE__, __LINE__); \
	fprintf(stdout, __VA_ARGS__); \
	fputc('\n', stdout); \
}
#else
#define error(...) { serprint(__VA_ARGS__); };
#define output(...) { serprint(__VA_ARGS__); };
#endif

#ifdef DEBUG
#ifdef POSIX
#define debug(...) { \
	fprintf(stderr, "%s:%d: ", __FILE__, __LINE__); \
	fprintf(stderr, __VA_ARGS__); \
	fputc('\n', stderr); \
}
#else
#define debug(...) { serprint(__VA_ARGS__); };
#endif
#else
#define debug(...) {}
#endif

#ifdef POSIX
#include <stdbool.h>
typedef bool boolean;
typedef unsigned char byte;
#else
#include <Arduino.h>
#endif

struct colour {
	byte r;
	byte g;
	byte b;
};

enum propertytype {
	NONE,
	FILL,
	ROTATION,
	OFFSET,
	HUE,
	LIGHTNESS,
	HUE2,
	LIGHTNESS2,
	PROPERTIES
};

#endif
