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

	animation_clear(a);
	return a;
}

void animation_clear(struct animation *a)
{
	enum propertytype p;

	for (p = 0; p < PROPERTIES; p++) {
		a->ap[p].constant = 0;
		a->ap[p].min = 0;
		a->ap[p].max = 0; /* disabled */
		a->ap[p].step = 1;
		a->ap[p].divider = 0; /* disabled */

		a->ap[p].mirror = false;
		a->ap[p].bounce = false;
		a->ap[p].reverse = false;
		a->ap[p].jog = false;

		a->ap[p].value = 0;
		a->ap[p].bouncing = false;
	}

	a->segments = 1;
	a->fill = SOLID;

	a->fg.r = 0;
	a->fg.g = 0;
	a->fg.b = 0;

	a->bg.r = 0;
	a->bg.g = 0;
	a->bg.b = 0;

	a->animate = NONE;
}

static void move(struct property *p, byte size, unsigned int delta)
{
	int new = p->value;

	if (p->reverse == p->bouncing)
		new += delta;
	else
		new -= delta;

	if (new < 0) {
		if (p->bounce) {
			p->bouncing = !p->bouncing;
			p->value = -new;
		} else {
			p->value = new + size;
		}

		return;
	}

	if (new > size) {
		if (p->bounce) {
			p->bouncing = !p->bouncing;
			p->value = size - (new - size);
		} else {
			p->value -= size;
		}

		return;
	}

	debug("new %d", new);
	p->value = new;
}

/*
 * Advance a property by one "frame".
 */
void animation_tock(struct animation *a, enum propertytype p,
						unsigned int delta)
{
	switch (p) {
	case FILL:
		move(&a->ap[p], RING_PIXELS, delta);
		break;

	case OFFSET:
		move(&a->ap[p], RING_PIXELS / a->segments, delta);
		break;

	case ROTATION:
		move(&a->ap[p], RING_PIXELS, delta);
		break;

	case NONE:
		break;

	default:
		error("invalid animate");
	}
}

void animation_sync(struct animation *a, boolean end)
{
	enum propertytype p;

	for (p = 0; p < PROPERTIES; p++) {
		if (end)
			/* FIXME: when no max set */
			a->ap[p].value = a->ap[p].max;
		else
			a->ap[p].value = a->ap[p].min;
	}
}

/*
 * Normalised to ensure the time to complete a ring is the same when step
 * or segments changes. Without this rings would fill twice as fast with two
 * segments or a step of two.
 *
 * FIXME: may need tweaking to what feels sensible after using live.
 *
 * A ring always takes RING_PIXELS clocks to complete a fill or rotation.
 * To perform this normalisation we must advance the value of a property
 * by (segments * steps) every (segments * steps) clock cycles.
 *
 * FIXME: normalise when min/max used to ensure region fills in RING_PIXELS
 * steps
 */
static unsigned int period(struct animation *a, enum propertytype p)
{
	switch (p) {
	case NONE:
		return 1;

	case FILL:
		return a->ap[p].step * a->segments;

	case ROTATION:
	case OFFSET:
		return a->ap[p].step;

	default:
		error("bad");
		return 1;
	}
}

/*
 * Manual advance triggered by a user button or sound event.
 * When divider = 0 then only jog will advance the property.
 *
 * FIXME: do we want to use dividers and a jog counter?
 */
void animation_jog(struct animation *a)
{
	enum propertytype p;

	for (p = 0; p < PROPERTIES; p++) {
		if (a->ap[p].jog)
			animation_tock(a, p, period(a, p));
	}
}

/*
 * Called every [scaled] global clock cycle.
 */
void animation_tick(struct animation *a, int clock)
{
	enum propertytype p;
	unsigned int d;

	for (p = 0; p < PROPERTIES; p++) {
		if (a->ap[p].divider == 0)
			continue;

		/* FIXME: may be incorrect */
		d = period(a, p);

		if (clock % (a->ap[p].divider * d) == 0)
			animation_tock(a, p, d);
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
