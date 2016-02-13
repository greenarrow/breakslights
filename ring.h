#ifndef RING_H
#define RING_H

#include "common.h"
#include "config.h"
#include "animation.h"

#include "pixel.h"

struct ring {
	byte offset;
	byte animation;
};

struct ring *ring_new(byte offset);
void ring_render(struct pixel *p, struct ring *r, struct animation *a);

#endif
