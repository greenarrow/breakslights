#ifndef MACHINE_H
#define MACHINE_H

#include "common.h"
#include "pixel.h"

struct machine {
	/* system state */
	unsigned int clock;

	struct pixel pixels;

	struct ring **rings;
	byte nrings;

	struct animation **animations;
	byte nanimations;

	/* modal animation state */
	byte chase_index;
	boolean strobe_on;

	byte chase_speed;
	byte strobe_speed;
};

void machine_set_rings(struct machine *m, const int n);
void machine_set_animations(struct machine *m, const int n);
struct ring *machine_get_ring(struct machine *m, byte i);
struct animation *machine_get_animation(struct machine *m, byte i);
void machine_init(struct machine *m);
void machine_destroy(struct machine *m);
void machine_tick(struct machine *m);
void machine_flush(struct machine *m);
void machine_assign(struct machine *m, struct ring *r, byte n);

#endif
