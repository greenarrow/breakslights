/*
 * Implementation of PROTOCOL.
 */

#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "animation.h"
#include "ring.h"
#include "machine.h"

#define DECA 48
#define DECB 57
#define HEXA 97
#define HEXB 102

byte power(byte a, byte b)
{
	byte i;
	byte result = a;

	if (b == 0)
		return 1;

	for (i = 0; i + 1 < b; i++)
		result *= a;

	return result;
}

char chrtobyte(const char c, byte *result, byte base)
{
	switch (base) {
	case 10:
		if (!(c >= DECA && c <= DECB))
			return -1;

		*result = c - DECA;
		break;

	case 16:
		if (!(c >= DECA && c <= DECB) && !(c >= HEXA && c <= HEXB))
			return -1;

		if (c <= DECB)
			*result = c - DECA;
		else
			*result = c - HEXA + 10;

		break;

	default:
		return -1;
	}

	return 0;
}

char strtobyte(const char *s, byte len, byte *result, byte base)
{
	byte value = 0;
	byte new;
	byte i;

	for (i = 0; i < len; i++) {
		if (chrtobyte(s[i], &new, base) == -1)
			return -1;

		value += new * power(base, len - i - 1);
	}

	*result = value;
	return 0;
}

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

static char readbyte(char **cursor, byte *result)
{
	if (strtobyte(*cursor, 3, result, 10) == -1)
		return -1;

	*cursor += 3;
	return 0;
}

static char readhex(char **cursor, byte *result)
{
	if (strtobyte(*cursor, 2, result, 16) == -1)
		return -1;

	*cursor += 2;
	return 0;
}

static char readcolour(char **cursor, struct colour *result)
{
	struct colour tmp;

	if (readhex(cursor, &tmp.r) == -1)
		return -1;

	if (readhex(cursor, &tmp.g) == -1)
		return -1;

	if (readhex(cursor, &tmp.b) == -1)
		return -1;

	*result = tmp;
	return 0;
}

static int handle_animation(char **cursor, char cmd, struct animation *a)
{
	switch (cmd) {
	case 'F':
		if (a == NULL)
			return -1;

		if (readcolour(cursor, &a->fg) == -1)
			return -1;

		break;

	case 'B':
		if (a == NULL)
			return -1;

		if (readcolour(cursor, &a->bg) == -1)
			return -1;

		break;

	case 'I':
		if (a == NULL)
			return -1;

		switch (readchar(cursor)) {
		case 'N':
			a->animate = NONE;
			break;

		case 'F':
			a->animate = FILL;
			break;

		case 'O':
			a->animate = OFFSET;
			break;

		case 'R':
			a->animate = ROTATION;
			break;

		default:
			return -1;
		}

		break;

	case 'P':
		if (a == NULL)
			return -1;

		if (readbyte(cursor, &a->ap[a->animate].step) == -1)
			return -1;

		break;

	case 'D':
		if (a == NULL)
			return -1;

		byte value;
		if (readbyte(cursor, &value) == -1)
			return -1;

		a->ap[a->animate].divider = 256 - value;

		break;

	case 'S':
		if (a == NULL)
			return -1;

		if (readbyte(cursor, &a->segments) == -1)
			return -1;
		break;

	case 'L':
		if (a == NULL)
			return -1;

		if (readbyte(cursor, &a->ap[FILL].constant) == -1)
			return -1;

		break;

	case 'O':
		if (a == NULL)
			return -1;

		if (readbyte(cursor, &a->ap[OFFSET].constant) == -1)
			return -1;

		break;

	case 'N':
		if (a == NULL)
			return -1;

		if (readbyte(cursor, &a->ap[ROTATION].constant) == -1)
			return -1;

		break;

	case 'E':
		if (a == NULL)
			return -1;

		a->ap[a->animate].bounce = readbool(cursor);
		break;

	case 'Z':
		if (a == NULL)
			return -1;

		a->ap[a->animate].mirror = readbool(cursor);
		break;

	case 'V':
		if (a == NULL)
			return -1;

		animation_jog(a);
		break;

	case 'C':
		if (a == NULL)
			return -1;

		animation_clear(a);
		break;

	case 'T':
		if (a == NULL)
			return -1;

		animation_sync(a, false);
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
	byte value;

	switch (cmd) {
	case 'N':
		if (readbyte(cursor, &value) == -1)
			return -1;

		machine_assign(m, r, value);
		break;

	default:
		return -1;
	}

	return 0;
}

static int handle_modal(struct machine *m, char **cursor, char cmd)
{
	byte value;

	switch (readchar(cursor)) {
	case 'A':
		if (readbyte(cursor, &value) == -1)
			return -1;

		machine_set_animations(m, value);
		break;

	case 'R':
		if (readbyte(cursor, &value) == -1)
			return -1;

		machine_set_rings(m, value);
		break;

	case 'S':
		if (readbyte(cursor, &m->strobe_speed) == -1)
			return -1;

		break;

	case 'B':
	case 'D':
		if (readbyte(cursor, &m->chase_speed) == -1)
			return -1;

		break;

	case 'I':
		output("D%.3u\n", m->missed);
		break;

	case 'F':
	case 'T':
		/* manual tick */
		debug("manual tick");
		machine_tick(m);
		machine_flush(m);
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
	byte value;

	while (cursor < end) {
		cmd = readchar(&cursor);

		switch (cmd) {
		case 'A':
			/* begin animation */
			r = NULL;

			if (readbyte(&cursor, &value) == -1)
				return -1;

			a = machine_get_animation(m, value);
			break;

		case 'R':
			/* begin ring */
			a = NULL;

			if (readbyte(&cursor, &value) == -1)
				return -1;

			r = machine_get_ring(m, value);
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
