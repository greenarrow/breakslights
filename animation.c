/*
 * An animation consists of a number of properties that result in a number
 * of frames. An animation can be drawn on one or more rings.
 */

#include <stdlib.h>

#include "common.h"
#include "config.h"
#include "animation.h"
#include "pixel.h"

#define HUE_REGION 40

struct animation *animation_new()
{
	struct animation *a = malloc(sizeof(struct animation));

	if (a == NULL) {
		error("malloc error");
		return NULL;
	}

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

		a->ap[p].bounce = false;
		a->ap[p].reverse = false;
		a->ap[p].jog = false;

		a->ap[p].value = 0;
		a->ap[p].bouncing = false;
	}

	a->segments = 1;
	a->mirror = false;
	a->fill = SOLID;
}

static inline byte incline(unsigned int region, unsigned int value)
{
	return (value - (region * HUE_REGION)) * 255 / HUE_REGION;
}

static inline byte decline(unsigned int region, unsigned int value)
{
	return 255 - ((value - (region * HUE_REGION)) * 255 / HUE_REGION);
}

/*
 * Pseudo HSL to RGB transform assuming always Smax.
 *
 * Hue is split into 6 regions where each of RGB will be one of
 * 0, max, incline or decline (both linear).
 *
 * We then apply a crude lightness transform by darkening for L < 127
 * or washing out for L > 127.
 *
 * L = 0 always gives black and L = 255 always gives white.
 * Brightness is not normalised across hue.
 *
 * H >= 0 < 6 * HUE_REGION
 * L >= 0 <= 255
 *
 * HUE_REGION is an arbitrary slicing of the colour space selected to fit
 * into the integer type.
 *
 * https://en.wikipedia.org/wiki/Hue#Computing_hue_from_RGB
 */
static struct colour hltorgb(byte h, byte l)
{
	struct colour result;

	unsigned int r, g, b;
	byte region = h / HUE_REGION;

	switch (region) {
	case 0:
		r = 255;
		g = incline(region, h);
		b = 0;
		break;

	case 1:
		r = decline(region, h);
		g = 255;
		b = 0;
		break;

	case 2:
		r = 0;
		g = 255;
		b = incline(region, h);
		break;

	case 3:
		r = 0;
		g = decline(region, h);
		b = 255;
		break;

	case 4:
		r = incline(region, h);
		g = 0;
		b = 255;
		break;

	case 5:
		r = 255;
		g = 0;
		b = decline(region, h);
		break;

	default:
		r = 0;
		g = 0;
		b = 0;
		error("invalid hue %u", h);
	}

	if (l < 127) {
		r -= r * (128 -l) / 128;
		g -= g * (128 -l) / 128;
		b -= b * (128 -l) / 128;
	}

	if (l > 127) {
		r += (255 - r) * (l - 128) / 127;
		g += (255 - g) * (l - 128) / 127;
		b += (255 - b) * (l - 128) / 127;
	}

	result.r = r;
	result.g = g;
	result.b = b;

	return result;
}

/*
 * Wrap around ends of block of size in both directions
 */
static int wrap(int value, int size)
{
	while (value >= size)
		value -= size;

	while (value < 0)
		value += size;

	return value;
}

static void clear(struct pixel *p, unsigned int bufp, struct colour c)
{
	byte i;

	for (i = 0; i < RING_PIXELS; i++)
		pixel_set(p, (bufp * RING_PIXELS) + wrap(i, RING_PIXELS), c);
}

static void draw(struct pixel *p, unsigned int bufp, struct animation *a,
				unsigned int segoff, bool mirror)
{
	/* FIXME: add support for segment mirroring */
	unsigned int start, stop;
	byte i;

	start = a->ap[OFFSET].value + a->ap[ROTATION].value + segoff;

	if (mirror) {
		start += (RING_PIXELS / a->segments) -
					(a->ap[FILL].value / a->segments);
	}

	start = wrap(start, RING_PIXELS);

	stop = start + (a->ap[FILL].value / a->segments);

	if (stop - start > RING_PIXELS / a->segments)
		stop -= RING_PIXELS / a->segments;

	for (i = start; i < stop; i++)
		pixel_set(p, (bufp * RING_PIXELS) + wrap(i, RING_PIXELS),
			hltorgb(a->ap[HUE].value, a->ap[LIGHTNESS].value));
}

void animation_render(struct pixel *p, unsigned int bufp, struct animation *a)
{
	struct colour black = {0, 0, 0};
	byte i;

	if (a == NULL) {
		clear(p, bufp, black);
		return;
	}

	clear(p, bufp, hltorgb(a->ap[HUE2].value, a->ap[LIGHTNESS2].value));

	for (i = 0; i < a->segments; i++) {
		draw(p, bufp, a, i * RING_PIXELS / a->segments,
						a->mirror && (i % 2));
	}
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
 * Return maximum allowed value of property.
 */
static byte limit(struct animation *a, enum propertytype p)
{
	switch (p) {
	case FILL:
		return RING_PIXELS;

	case ROTATION:
		return RING_PIXELS - 1;

	case OFFSET:
		return (RING_PIXELS / a->segments) - 1;

	case HUE:
	case HUE2:
		return 239;

	case LIGHTNESS:
	case LIGHTNESS2:
		return 255;

	default:
		error("invalid animate");
	}

	return 0;
}

/*
 * Advance a property by one "frame".
 */
void animation_tock(struct animation *a, enum propertytype p,
						unsigned int delta)
{
	move(&a->ap[p], limit(a, p), delta);
}

void animation_sync(struct animation *a, bool end)
{
	enum propertytype p;

	for (p = 0; p < PROPERTIES; p++) {
		if (end) {
			if (a->ap[p].max == 0)
				a->ap[p].value = limit(a, p);
			else
				a->ap[p].value = a->ap[p].max;
		} else {
			a->ap[p].value = a->ap[p].min;
		}
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

	case HUE:
	case HUE2:
		return a->ap[p].step;

	case LIGHTNESS:
	case LIGHTNESS2:
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
		/* property is not animated */
		if (a->ap[p].divider == 0) {
			a->ap[p].value = a->ap[p].constant;
			continue;
		}

		/* FIXME: may be incorrect */
		d = period(a, p);

		/* property is animated */
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
