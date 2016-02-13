/*
 * Global state of the display. A single machine instance is used.
 */

#include <stdlib.h>

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
	m->nrings = n;

	for (i = 0; i < n; i++)
		m->rings[i] = ring_new(PIN_START + i);

	debug("set %d rings", n);
}

void machine_set_animations(struct machine *m, const int n)
{
	int i;

	free_animations(m);

	m->animations = malloc(n * sizeof(struct animation*));
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

	m->rings = NULL;
	m->nrings = 0;

	m->animations = NULL;
	m->nanimations = 0;

	m->chase_index = 0;
	m->strobe_on = true;

	m->chase_speed = 0;
	m->strobe_speed = 0;
}

void machine_destroy(struct machine *m)
{
	free_rings(m);
	free_animations(m);
}

void machine_assign(struct machine *m, struct ring *r, byte n)
{
	if (machine_get_animation(m, n) == NULL)
		return;

	r->animation = n;

	debug("assign animation %d", n);
}

static boolean tock(struct machine *m, byte speed)
{
	if (speed == 0)
		return false;

	if ((m->clock % (256 - speed)) == 0)
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

	m->clock++;

	if (!m->strobe_on && m->strobe_speed == 0)
		m->strobe_on = true;

	if (tock(m, m->strobe_speed))
		m->strobe_on = !m->strobe_on;

	/* FIXME: implement reflection */
	if (tock(m, m->chase_speed)) {
		m->chase_index++;

		if (m->chase_index >= m->nrings)
			m->chase_index = 0;
	}

	/* always step animations even if not visible */
	for (i = 0; i < m->nanimations; i++) {
		if (tock(m, m->animations[i]->speed))
			animation_step(m->animations[i]);
	}

	/* FIXME: add more granular redraw */
	for (i = 0; i < m->nrings; i++) {
		/* FIXME: chase mapping */
		ring_render(m->rings[i],
			m->strobe_on ? machine_get_animation(m,
			chased(m, i)->animation) : NULL);
	}
}

void machine_flush(struct machine *m)
{
	int i;

	for (i = 0; i < m->nrings; i++)
		ring_flush(m->rings[i], i);
}
