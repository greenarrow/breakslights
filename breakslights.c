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
	char *line = NULL;
	size_t len = 0;

	debug("init");
	machine_init(&m);

	debug("loop");

	for (;;) {
		debug("waiting");
#ifdef POSIX
		if (getline(&line, &len, stdin) == -1)
			break;

		handle_line(&m, line);
#endif
	}

	if (line)
		free(line);

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
