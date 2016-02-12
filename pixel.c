/*
 */
#include "config.h"

#ifndef POSIX
#include "neopixel.h"
#endif

#include "pixel.h"

void pixel_init(struct pixel *p, byte pin)
{
	int i;

#ifndef POSIX
	ledsetup();
#endif

	for (i = 0; i < RING_PIXELS * 3; i++)
		p->pixels[i] = 0;
}

void pixel_destroy(struct pixel *p)
{
}

void pixel_set(struct pixel *p, byte n, struct colour c)
{
	p->pixels[n * 3] = c.g;
	p->pixels[n * 3 + 1] = c.r;
	p->pixels[n * 3 + 2] = c.b;
}

void pixel_flush(struct pixel *p)
{
	int i;

#ifndef POSIX
	for (i = 0; i < RING_PIXELS * 3; i++)
		sendByte(p->pixels[i]);

	show();
#endif
}
