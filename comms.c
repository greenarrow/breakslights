/*
 * Implementation of PROTOCOL.
 */

#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "animation.h"
#include "ring.h"
#include "machine.h"

static char readchar(char **cursor)
{
	char value = *cursor[0];
	*cursor += 1;

	return value;
}

static boolean readbool(char **cursor)
{
	if (readchar(cursor) == '1')
		return true;

	return false;
}

static byte readbyte(char **cursor)
{
	char buffer[4];
	strncpy(buffer, *cursor, 3);
	buffer[3] = '\0';

	*cursor += 3;

	return atoi(buffer);
}

static byte readhex(char **cursor)
{
	char buffer[3];

	strncpy(buffer, *cursor, 2);
	*cursor += 2;
	buffer[2] = '\0';

	return strtol(buffer, NULL, 16);
}

static void readcolour(char **cursor, volatile struct colour *c)
{
	c->r = readhex(cursor);
	c->g = readhex(cursor);
	c->b = readhex(cursor);
}

static int handle_animation(char **cursor, char cmd, struct animation *a)
{
	switch (cmd) {
	case 'F':
		if (a == NULL)
			return -1;

		readcolour(cursor, &a->fg);
		break;

	case 'B':
		if (a == NULL)
			return -1;

		readcolour(cursor, &a->bg);
		break;

	case 'I':
		if (a == NULL)
			return -1;

		switch (readchar(cursor)) {
		case 'N':
			a->animate = P_NONE;
			break;

		case 'F':
			a->animate = P_FILL;
			break;

		case 'O':
			a->animate = P_OFFSET;
			break;

		case 'R':
			a->animate = P_ROTATION;
			break;

		default:
			return -1;
		}

		break;

	case 'P':
		if (a == NULL)
			return -1;

		a->step = readbyte(cursor);
		break;

	case 'D':
		if (a == NULL)
			return -1;

		a->speed = readbyte(cursor);
		break;

	case 'S':
		if (a == NULL)
			return -1;

		a->segments = readbyte(cursor);
		break;

	case 'L':
		if (a == NULL)
			return -1;

		a->fill = readbyte(cursor);
		break;

	case 'O':
		if (a == NULL)
			return -1;

		a->offset = readbyte(cursor);
		break;

	case 'N':
		if (a == NULL)
			return -1;

		a->rotation = readbyte(cursor);
		break;

	case 'E':
		if (a == NULL)
			return -1;

		a->bounce = readbool(cursor);
		break;

	case 'Z':
		if (a == NULL)
			return -1;

		a->mirror = readbool(cursor);
		break;

	case 'V':
		if (a == NULL)
			return -1;

		a->frame++;
		break;

	default:
		return -1;
	}

	if (a)
		animation_validate(a);

	return 0;
}

static int handle_ring(struct machine *m, char **cursor, char cmd,
							struct ring *r)
{
	switch (cmd) {
	case 'N':
		machine_assign(m, r, readbyte(cursor));
		break;

	default:
		return -1;
	}

	return 0;
}

static int handle_modal(struct machine *m, char **cursor, char cmd)
{
	switch (readchar(cursor)) {
	case 'A':
		machine_set_animations(m, readbyte(cursor));
		break;

	case 'R':
		machine_set_rings(m, readbyte(cursor));
		break;

	case 'S':
		m->strobe_speed = readbyte(cursor);
		break;

	case 'B':
	case 'D':
		m->chase_speed = readbyte(cursor);
		break;

	case 'F':
	case 'T':
		/* manual tick */
		debug("manual tick");
		machine_tick(m);
		break;

	/* FIXME add animations sync to call animation_reset() on all */

	default:
		return -1;
	}

	return 0;
}

int handle_line(struct machine *m, char *line)
{
	char *cursor = line;
	char *end = line + strlen(line);
	struct animation *a = NULL;
	struct ring *r = NULL;
	char cmd;

	while (cursor < end) {
		cmd = readchar(&cursor);

		switch (cmd) {
		case 'A':
			/* begin animation */
			r = NULL;
			a = machine_get_animation(m, readbyte(&cursor));
			break;

		case 'R':
			/* begin ring */
			a = NULL;
			r = machine_get_ring(m, readbyte(&cursor));
			break;

		case 'M':
			/* modal command */
			a = NULL;
			r = NULL;

			if (handle_modal(m, &cursor, cmd) == -1)
				goto error;

			break;

		case '#':
			return 0;

		case '\n':
			return 0;

		default:
			if ((a == NULL) == (r == NULL))
				goto error;

			if (a) {
				if (handle_animation(&cursor, cmd, a) == -1)
					goto error;
			}

			if (r) {
				if (handle_ring(m, &cursor, cmd, r) == -1)
					goto error;
			}

			break;
		}

		switch (readchar(&cursor)) {
		case ' ':
			continue;

		case '\n':
			return 0;

		default:
			goto error;
		}
	}

error:
	error("invalid line: %s", line);
	return -1;
}
