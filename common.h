#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdarg.h>

#ifdef POSIX
#define error(...) { \
	fprintf(stderr, "%s:%d: ", __FILE__, __LINE__); \
	fprintf(stderr, __VA_ARGS__); \
	fputc('\n', stderr); \
}
#endif

#ifdef DEBUG
#ifdef POSIX
#define debug(...) { \
	fprintf(stderr, "%s:%d: ", __FILE__, __LINE__); \
	fprintf(stderr, __VA_ARGS__); \
	fputc('\n', stderr); \
}
#endif
#else
#define debug(...) {}
#endif

#ifdef POSIX
#include <stdbool.h>
typedef bool boolean;
typedef unsigned char byte;
#endif

struct colour {
	byte r;
	byte g;
	byte b;
};

#endif
