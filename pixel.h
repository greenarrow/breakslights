#ifndef PIXEL_H
#define PIXEL_H

#include "config.h"
#include "common.h"

struct pixel {
	byte len;
	unsigned char *pixels;
};

void pixel_init(struct pixel *p, byte pin);
void pixel_destroy(struct pixel *p);
void pixel_set(struct pixel *p, byte n, struct colour c);
void pixel_flush(struct pixel *p);

#endif
