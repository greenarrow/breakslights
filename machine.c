/*
 * Global state of the display. A single machine instance is used.
 */

#include <stdlib.h>
#include <stdbool.h>

#include "common.h"
#include "config.h"
#include "ring.h"
#include "animation.h"
#include "machine.h"

static void free_rings(struct machine *m)
{
	int i;

	for (i = 0; i < m->nrings; i++)
		free(m->rings[i]);

	if (m->rings)
		free(m->rings);

	m->nrings = 0;
}

static void free_animations(struct machine *m)
{
	int i;

	for (i = 0; i < m->nanimations; i++)
		free(m->animations[i]);

	if (m->animations)
		free(m->animations);

	m->nanimations = 0;
}

void machine_set_rings(struct machine *m, const int n)
{
	int i;

	free_rings(m);
	m->rings = malloc(n * sizeof(struct ring*));

	if (m->rings == NULL) {
		error("malloc error");
		return;
	}

	m->nrings = n;

	for (i = 0; i < n; i++)
		m->rings[i] = ring_new(i);

	pixel_destroy(&m->pixels);
	pixel_init(&m->pixels, RING_PIXELS * n);

	debug("set %d rings", n);
}

void machine_set_animations(struct machine *m, const int n)
{
	int i;

	free_animations(m);
	m->animations = malloc(n * sizeof(struct animation*));

	if (m->animations == NULL) {
		error("malloc error");
		return;
	}

	m->nanimations = n;

	for (i = 0; i < n; i++)
		m->animations[i] = animation_new();

	debug("set %d animations", n);
}

struct ring *machine_get_ring(struct machine *m, byte i)
{
	if (i >= m->nrings) {
		error("invalid ring index: %d", i);
		return NULL;
	}

	return m->rings[i];
}

struct animation *machine_get_animation(struct machine *m, byte i)
{
	if (i >= m->nanimations) {
		error("invalid animation index: %d", i);
		return NULL;
	}

	return m->animations[i];
}

void machine_init(struct machine *m)
{
	m->clock = 0;
	m->subticks = 0;

	m->rings = NULL;
	m->nrings = 0;

	m->animations = NULL;
	m->nanimations = 0;

	m->divider = 1;

	m->chase_index = 0;
	m->strobe_on = true;

	m->chase_divider = 0;
	m->strobe_divider = 0;
	m->master_fade = 255;

	m->missed = 0;

	pixel_init(&m->pixels, 0);
}

void machine_destroy(struct machine *m)
{
	free_rings(m);
	free_animations(m);
	pixel_destroy(&m->pixels);
}

void machine_assign(struct machine *m, struct ring *r, byte n)
{
	if (machine_get_animation(m, n) == NULL)
		return;

	r->animation = n;

	debug("assign animation %d", n);
}

static bool tock(struct machine *m, byte divider)
{
	if (divider == 0)
		return false;

	if ((m->clock % divider) == 0)
		return true;

	return false;
}

static struct ring *chased(struct machine *m, byte r)
{
	byte pos = r + m->chase_index;

	while (pos >= m->nrings)
		pos -= m->nrings;

	return machine_get_ring(m, pos);
}

void machine_tick(struct machine *m)
{
	int i;

	if (m->divider > 0) {
		m->subticks++;

		if (m->subticks < m->divider)
			return;

		m->subticks = 0;
	}

	m->clock++;

	if (!m->strobe_on && m->strobe_divider == 0)
		m->strobe_on = true;

	if (tock(m, m->strobe_divider))
		m->strobe_on = !m->strobe_on;

	/* FIXME: implement reflection */
	if (tock(m, m->chase_divider)) {
		m->chase_index++;

		if (m->chase_index >= m->nrings)
			m->chase_index = 0;
	}

	/* always step animations even if not visible */
	for (i = 0; i < m->nanimations; i++)
		animation_tick(m->animations[i], m->clock);

	/* FIXME: add more granular redraw */
	for (i = 0; i < m->nrings; i++) {
		/* FIXME: chase mapping */
		animation_render(&m->pixels, m->rings[i]->offset,
			m->strobe_on ? machine_get_animation(m,
			chased(m, i)->animation) : NULL);
	}
}

void machine_flush(struct machine *m)
{
	pixel_flush(&m->pixels);
}
