/*
 * A ring represents on physical ring of LEDs.
 */

#include <stdlib.h>

#include "common.h"
#include "config.h"
#include "animation.h"
#include "ring.h"

#ifndef POSIX
#include "pixel.h"
#endif

void ring_flush(struct ring *r, byte addr)
{
#ifdef POSIX
	int i;

	printf("%.2x ", addr);

	for (i = 0; i < RING_PIXELS; i++) {
		printf("%.2x%.2x%.2x", r->pixels[i].r, r->pixels[i].g,
							r->pixels[i].b);
		if (i + 1 < RING_PIXELS)
			printf(" ");
	}

	printf("\n");
#else
	pixel_flush(&r->pixels);
#endif
}

struct ring *ring_new(byte pin)
{
	struct ring *r = malloc(sizeof(struct ring));

	if (r == NULL)
		return NULL;

	r->animation = 0;

#ifndef POSIX
	pixel_init(&r->pixels, pin);
#endif

	return r;
}

static void setpixel(struct ring *r, byte p, struct colour c)
{
	/* wrap around ring */
	while (p >= RING_PIXELS)
		p -= RING_PIXELS;

#ifdef POSIX
	r->pixels[p] = c;
#else
	pixel_set(&r->pixels, p, c);
#endif
}

static void clear(struct ring *r, struct colour c)
{
	byte i;

	for (i = 0; i < RING_PIXELS; i++)
		setpixel(r, i, c);
}

static void draw(struct ring *r, struct animation *a, unsigned int fill,
				unsigned int offset, unsigned int rotation,
				boolean mirror)
{
	unsigned int start, stop;
	byte i;

	start = a->offset + a->rotation + offset + rotation;

	while (start >= RING_PIXELS)
		start -= RING_PIXELS;

	if (mirror)
		start = start + RING_PIXELS / a->segments - fill;

	stop = start + fill;

	if (stop - start > RING_PIXELS / a->segments)
		stop -= RING_PIXELS / a->segments;

	for (i = start; i < stop; i++)
		setpixel(r, i, a->fg);
}

void ring_render(struct ring *r, struct animation *a)
{
	struct colour black = {0, 0, 0};
	unsigned int fill = 0;
	unsigned int offset = 0;
	unsigned int rotation = 0;
	byte i;

	if (a == NULL) {
		clear(r, black);
		return;
	}

	/* fill is animated or static not both */
	fill = a->fill;

	switch (a->animate) {
	case P_FILL:
		fill = a->frame;
		break;

	case P_OFFSET:
		offset = a->frame;
		break;

	case P_ROTATION:
		rotation = a->frame;
		break;

	case P_NONE:
		fill = a->fill;
		break;

	default:
		error("invalid animate");
	}

	clear(r, a->bg);

	for (i = 0; i < a->segments; i++) {
		draw(r, a, fill, offset,
				rotation + i * RING_PIXELS / a->segments,
				a->mirror && (i % 2));
	}
}
