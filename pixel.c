/*
 */
#include <stdlib.h>
#include "config.h"

#ifndef POSIX
#include "neopixel.h"
#endif

#include "pixel.h"

void pixel_init(struct pixel *p, byte size)
{
	int i;

#ifndef POSIX
	ledsetup();
#endif
	debug("buffer size %d\n", sizeof(unsigned char) * 3 * size + 1);

	if (size == 0) {
		p->pixels = NULL;
		return;
	}

	p->len = size;
	p->pixels = malloc(sizeof(unsigned char) * 3 * size + 1);

	for (i = 0; i < RING_PIXELS * 3; i++)
		p->pixels[i] = 0;
}

void pixel_destroy(struct pixel *p)
{
	if (p->pixels == NULL)
		return;

	free(p->pixels);
	p->pixels = NULL;
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

#ifdef POSIX
	int r;
	int s;

	for (r = 0; r < p->len / RING_PIXELS; r++) {
		printf("%.2x ", r);

		for (i = 0; i < RING_PIXELS; i++) {
			s = (r * RING_PIXELS) + i;

			printf("%.2x%.2x%.2x", p->pixels[s * 3 + 1],
					p->pixels[s * 3], p->pixels[s * 3 + 2]);

			if (i + 1 < RING_PIXELS)
				printf(" ");
		}

		printf("\n");
	}
#else
	for (i = 0; i < p->len * 3; i++)
		sendByte(p->pixels[i]);

	show();
#endif
}
