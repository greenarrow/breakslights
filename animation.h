#ifndef ANIMATION_H
#define ANIMATION_H

#include "common.h"
#include "pixel.h"

#include <stdbool.h>

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

	bool bounce;
	bool reverse;
	bool jog;

	/* runtime state */
	byte value;
	bool bouncing;
};

struct animation {
	/* animate-able properties */
	struct property ap[PROPERTIES];

	byte segments;
	bool mirror;
	enum filltype fill;
};

struct animation *animation_new();
void animation_clear(struct animation *a);
struct colour hltorgb(byte h, byte l);
void animation_render(struct pixel *p, unsigned int bufp, struct animation *a);
void animation_sync(struct animation *a, bool end);
void animation_jog(struct animation *a);
void animation_tick(struct animation *a, int clock);
void animation_validate(struct animation *a);

#endif
