#ifndef ANIMATION_H
#define ANIMATION_H

#include "common.h"
#include "pixel.h"

enum filltype {
	SOLID,
	LINEAR
};

struct property {
	/* user values */
	byte constant;
	byte min;
	byte max;
	byte step;
	byte divider;

	boolean bounce;
	boolean reverse;
	boolean jog;

	/* runtime state */
	byte value;
	boolean bouncing;
};

struct animation {
	/* animate-able properties */
	struct property ap[PROPERTIES];

	byte segments;
	boolean mirror;
	enum filltype fill;
};

struct animation *animation_new();
void animation_clear(struct animation *a);
void animation_render(struct pixel *p, unsigned int bufp, struct animation *a);
void animation_sync(struct animation *a, boolean end);
void animation_jog(struct animation *a);
void animation_tick(struct animation *a, int clock);
void animation_validate(struct animation *a);

#endif
