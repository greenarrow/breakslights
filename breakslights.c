/*
 * Main program builds for Arduino or for POSIX using standard input and
 * output when POSIX is defined.
 */

#include <stdlib.h>

#include "machine.h"
#include "comms.h"

struct machine m;

void setup()
{
}

void loop()
{
#ifdef POSIX
	char *line = NULL;
	size_t len = 0;
#else
	char buf[256];
	byte nb = 0;
#endif
	debug("init");
	machine_init(&m);

#ifndef POSIX
        output("breakslights");
#endif

	debug("loop");

	for (;;) {
		debug("waiting");
#ifdef POSIX
		if (getline(&line, &len, stdin) == -1)
			break;

		handle_line(&m, line);
#else
	nb = serial_getdelim('\n', buf, 255);

	if (nb == 0) {
		error("null read");
		continue;
	}

	if (handle_line(&m, buf) == -1) {
		output("1");
	} else {
		output("0");
	}
#endif
	}

#ifdef POSIX
	if (line)
		free(line);
#endif

	machine_destroy(&m);
}

#ifdef POSIX
int main()
{
	setup();
	loop();
	return 0;
}
#endif
