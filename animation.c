/*
 * An animation consists of a number of properties that result in a number
 * of frames. An animation can be drawn on one or more rings.
 */

#include <stdlib.h>

#include "common.h"
#include "config.h"
#include "animation.h"

struct animation *animation_new()
{
	struct animation *a = malloc(sizeof(struct animation));

	if (a == NULL)
		return NULL;

	a->fg.r = 0;
	a->fg.g = 0;
	a->fg.b = 0;

	a->bg.r = 0;
	a->bg.g = 0;
	a->bg.b = 0;

	a->animate = P_NONE;

	a->segments = 1;
	a->fill = 0;
	a->offset = 0;
	a->rotation = 0;
	a->mirror = false;

	a->step = 0;
	a->speed = 0;

	a->reverse = false;
	a->bounce = false;

	a->frame = 0;
	a->backwards = false;

	return a;
}

void animation_reset(struct animation *a)
{
	a->frame = 0;
}

static void move(struct animation *a, byte size)
{
	int new = a->frame;

	if (a->backwards)
		new -= a->step;
	else
		new += a->step;

	if (new < 0) {
		if (a->bounce) {
			a->backwards = !a->backwards;
			a->frame = new + a->step * 2;
		} else {
			a->frame = new + size;
		}

		goto finish;
	}

	if (new > size) {
		if (a->bounce) {
			a->backwards = !a->backwards;
			a->frame = new - a->step * 2;
		} else {
			a->frame -= size;
		}

		goto finish;
	}

	a->frame = new;

finish:
	debug("frame %d", new);
}

void animation_step(struct animation *a)
{
	switch (a->animate) {
	case P_FILL:
		move(a, RING_PIXELS / a->segments);
		break;

	case P_OFFSET:
		move(a, RING_PIXELS / a->segments);
		break;

	case P_ROTATION:
		move(a, RING_PIXELS);
		break;

	case P_NONE:
		break;

	default:
		error("invalid animate");
	}
}

void animation_validate(struct animation *a)
{
	if (a->segments > RING_PIXELS) {
		error("segments is larger than pixels %u > %u", a->segments,
								RING_PIXELS);
	}

	if (a->fill > RING_PIXELS / a->segments) {
		error("fill is larger than segment size %u > %u", a->fill,
						RING_PIXELS / a->segments);
	}
}
