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
	struct colour fg;
	struct colour bg;

	enum property animate;

	/* static pattern properties */
	byte segments;
	byte fill;
	byte offset;
	byte rotation;
	boolean mirror;

	/* animation properties */
	byte step;
	byte speed;
	boolean reverse;
	boolean bounce;

	/* current frame */
	byte frame;
	boolean backwards;
};

struct animation *animation_new();
void animation_clear(struct animation *a);
void animation_reset(struct animation *a);
void animation_step(struct animation *a);
void animation_validate(struct animation *a);

#endif
