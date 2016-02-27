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
