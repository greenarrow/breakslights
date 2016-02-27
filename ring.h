#ifndef RING_H
#define RING_H

#include "common.h"
#include "config.h"
#include "animation.h"

struct ring {
	byte offset;
	byte animation;
};

struct ring *ring_new(byte offset);

#endif
