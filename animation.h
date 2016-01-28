#ifndef ANIMATION_H
#define ANIMATION_H

#include "common.h"

enum property {
	P_NONE,
	P_FILL,
	P_OFFSET,
	P_ROTATION,
};

struct animation {
	volatile struct colour fg;
	volatile struct colour bg;

	volatile enum property animate;

	/* static pattern properties */
	volatile byte segments;
	volatile byte fill;
	volatile byte offset;
	volatile byte rotation;
	volatile boolean mirror;

	/* animation properties */
	volatile byte step;
	volatile byte speed;
	volatile boolean reverse;
	volatile boolean bounce;

	/* current frame */
	volatile byte frame;
	volatile boolean backwards;
};

struct animation *animation_new();
void animation_reset(struct animation *a);
void animation_step(struct animation *a);
void animation_validate(struct animation *a);

#endif
