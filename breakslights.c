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
#ifndef POSIX
	serial_init(9600);

	/* FIXME: select most appropriate frequency based on min / max desired
	* speeds; also consider logarithmic speeds. */

	/* FIXME: ensure bytes are not lost in serial communications. */

	/* initialize timer1 */
	noInterrupts();
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1  = 0;
	OCR1A = 521;		/* compare match register 16MHz/256/2Hz */
	TCCR1B |= (1 << WGM12);	/* CTC mode */
	TCCR1B |= (1 << CS12);	/* 256 prescaler */
	TIMSK1 |= (1 << OCIE1A);	/* enable timer compare interrupt */
	interrupts();
#endif
}

#ifndef POSIX
/* timer compare interrupt service routine */
ISR(TIMER1_COMPA_vect)
{
	machine_tick(&m);
	machine_flush(&m);
}
#endif

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
