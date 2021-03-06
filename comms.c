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

static bool readbool(char **cursor)
{
	if (readchar(cursor) == '1')
		return true;

	return false;
}

static char readhex(char **cursor, byte *result)
{
	if (strtobyte(*cursor, 2, result, 16) == -1)
		return -1;

	*cursor += 2;
	return 0;
}

static bool getproperty(const char p, byte *value)
{
	switch (p) {
	case 'F':
		*value = FILL;
		break;
	case 'O':
		*value = ROTATION;
		break;
	case 'T':
		*value = OFFSET;
		break;
	case 'H':
		*value = HUE;
		break;
	case 'B':
		*value = LIGHTNESS;
		break;
	case 'U':
		*value = HUE2;
		break;
	case 'G':
		*value = LIGHTNESS2;
		break;
	default:
		return false;
	}

	return true;
}

static int handle_property(char **cursor, char p, struct animation *a)
{
	byte i;
	char v;

	if (!getproperty(p, &i)) {
		debug("property %c does not exist", p);
		return -1;
	}

	v = readchar(cursor);

	switch (v) {
	case 'C':
		if (readhex(cursor, &a->ap[i].constant) == -1)
			return -1;

		break;

	case 'N':
		if (readhex(cursor, &a->ap[i].min) == -1)
			return -1;

		break;

	case 'X':
		if (readhex(cursor, &a->ap[i].max) == -1)
			return -1;

		break;

	case 'S':
		if (readhex(cursor, &a->ap[i].step) == -1)
			return -1;

		break;

	case 'D':
		if (readhex(cursor, &a->ap[i].divider) == -1)
			return -1;
		break;

	case 'B':
		a->ap[i].bounce = readbool(cursor);
		break;

	case 'V':
		a->ap[i].reverse = readbool(cursor);
		break;

	default:
		debug("value %c does not exist", v);
		return -1;
	}

	return 0;
}

static int handle_animation(char **cursor, char cmd, struct animation *a)
{
	switch (cmd) {
	case 'L':
		animation_clear(a);
		break;

	case 'Y':
		animation_sync(a, !readbool(cursor));
		break;

	case 'S':
		if (readhex(cursor, &a->segments) == -1)
			return -1;
		break;

	case 'I':
		a->mirror = readbool(cursor);
		break;

	case 'F':
	case 'O':
	case 'T':
	case 'E':
	case 'H':
	case 'B':
	case 'U':
	case 'G':
		if (handle_property(cursor, cmd, a) == -1) {
			debug("bad property %c", cmd);
			return -1;
		}

		break;

	default:
		debug("invalid command %c", cmd);
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
		if (readhex(cursor, &value) == -1)
			return -1;

		machine_assign(m, r, value);
		break;

	default:
		return -1;
	}

	return 0;
}

static int handle_modal(struct machine *m, char **cursor)
{
	byte value;

	switch (readchar(cursor)) {
	case 'A':
		if (readhex(cursor, &value) == -1)
			return -1;

		machine_set_animations(m, value);
		break;

	case 'R':
		if (readhex(cursor, &value) == -1)
			return -1;

		machine_set_rings(m, value);
		break;

	case 'D':
		if (readhex(cursor, &m->divider) == -1)
			return -1;
		break;

	case 'S':
		if (readhex(cursor, &m->strobe_divider) == -1)
			return -1;
		break;

	case 'C':
		if (readhex(cursor, &m->chase_divider) == -1)
			return -1;
		break;

	case 'I':
		output("C%u D%u", m->clock, m->missed);
		break;

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

			if (readhex(&cursor, &value) == -1)
				return -1;

			a = machine_get_animation(m, value);
			break;

		case 'R':
			/* begin ring */
			a = NULL;

			if (readhex(&cursor, &value) == -1)
				return -1;

			r = machine_get_ring(m, value);
			break;

		case 'M':
			/* modal command */
			a = NULL;
			r = NULL;

			if (handle_modal(m, &cursor) == -1) {
				debug("modal read error");
				goto error;
			}

			break;

		case '#':
			return 0;

		case '\n':
			return 0;

		default:
			if ((a == NULL) == (r == NULL))
				goto error;

			if (a) {
				if (handle_animation(&cursor, cmd, a) == -1) {
					debug("animation read error");
					goto error;
				}
			}

			if (r) {
				if (handle_ring(m, &cursor, cmd, r) == -1) {
					debug("ring read error");
					goto error;
				}
			}

			break;
		}

		switch (readchar(&cursor)) {
		case ' ':
			continue;

		case '\n':
			return 0;

		default:
			debug("invalid command '%c'", cmd);
			goto error;
		}
	}

error:
	error("invalid line: %s", line);
	return -1;
}
