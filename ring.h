#ifndef RING_H
#define RING_H

#include "common.h"
#include "config.h"
#include "animation.h"

#ifndef POSIX
#include "pixel.h"
#endif

struct ring {
	byte animation;

#ifdef POSIX
	struct colour pixels[RING_PIXELS];
#else
	struct pixel pixels;
#endif
};

struct ring *ring_new(byte pin);
void ring_render(struct ring *r, struct animation *a);
void ring_flush(struct ring *r, byte addr);

#endif
