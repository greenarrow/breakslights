/*
 * A ring represents on physical ring of LEDs.
 */

#include <stdlib.h>

#include "common.h"
#include "config.h"
#include "animation.h"
#include "pixel.h"
#include "ring.h"

struct ring *ring_new(byte offset)
{
	struct ring *r = malloc(sizeof(struct ring));

	if (r == NULL)
		return NULL;

	r->animation = 0;
	r->offset = offset;

	return r;
}

static void setpixel(struct pixel *p, struct ring *r, byte px, struct colour c)
{
	/* wrap around ring */
	while (px >= RING_PIXELS)
		px -= RING_PIXELS;

	pixel_set(p, (r->offset * RING_PIXELS) + px, c);
}

static void clear(struct pixel *p, struct ring *r, struct colour c)
{
	byte i;

	for (i = 0; i < RING_PIXELS; i++)
		setpixel(p, r, i, c);
}

static void draw(struct pixel *p, struct ring *r, struct animation *a,
				unsigned int fill, unsigned int offset,
				unsigned int rotation, boolean mirror)
{
	unsigned int start, stop;
	byte i;

	start = a->ap[OFFSET].constant + a->ap[ROTATION].constant + offset +
								rotation;

	while (start >= RING_PIXELS)
		start -= RING_PIXELS;

	if (mirror)
		start = start + RING_PIXELS / a->segments - fill;

	stop = start + fill;

	if (stop - start > RING_PIXELS / a->segments)
		stop -= RING_PIXELS / a->segments;

	for (i = start; i < stop; i++)
		setpixel(p, r, i, a->fg);
}

void ring_render(struct pixel *p, struct ring *r, struct animation *a)
{
	struct colour black = {0, 0, 0};
	unsigned int fill = 0;
	unsigned int offset = 0;
	unsigned int rotation = 0;
	byte i;

	if (a == NULL) {
		clear(p, r, black);
		return;
	}

	/* fill is animated or static not both */
	fill = a->ap[FILL].constant;

	switch (a->animate) {
	case FILL:
		fill = a->ap[FILL].value;
		break;

	case OFFSET:
		offset = a->ap[OFFSET].value;
		break;

	case ROTATION:
		rotation = a->ap[ROTATION].value;
		break;

	default:
		error("invalid animate");
	}

	clear(p, r, a->bg);

	for (i = 0; i < a->segments; i++) {
		draw(p, r, a, fill, offset,
				rotation + i * RING_PIXELS / a->segments,
				a->ap[a->animate].mirror && (i % 2));
	}
}
