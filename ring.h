#ifndef RING_H
#define RING_H

#include "common.h"
#include "config.h"
#include "animation.h"

struct ring {
	byte animation;

#ifdef POSIX
	struct colour pixels[RING_PIXELS];
#endif
};

struct ring *ring_new();
void ring_render(struct ring *r, struct animation *a);
void ring_flush(struct ring *r, byte addr);

#endif
