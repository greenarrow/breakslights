/*
 * Main program builds for AVR or for POSIX using standard input and
 * output when POSIX is defined.
 */

#include <stdlib.h>
#ifndef POSIX
#include <avr/interrupt.h>
#include <util/delay.h>
#endif

#include "machine.h"
#include "comms.h"
#include "uart.h"


/* more than enough time to catch 3 extra characters at 57600 baud */
#define UART_WAIT 1

struct machine m;
struct uart u;

void setup()
{
#ifndef POSIX
	cli();
	uart_init(&u);

	/* initialize timer1 */
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1  = 0;
	OCR1A = 521;		/* compare match register 16MHz/256/2Hz */
	TCCR1B |= (1 << WGM12);	/* CTC mode */
	TCCR1B |= (1 << CS12);	/* 256 prescaler */
	TIMSK1 |= (1 << OCIE1A);	/* enable timer compare interrupt */

	/*
	 * Disable unrequited timers to prevent interruption; this will break
	 * millis().
	 */
	TCCR0B = 0x0;
	TCCR2B = 0x0;

	sei();
#endif
}

volatile boolean draw = false;
volatile unsigned int missed = 0;

#ifndef POSIX
/* timer compare interrupt service routine */
ISR(TIMER1_COMPA_vect)
{
	if (draw && missed < 255)
		missed++;

	draw = true;
}
#endif

void loop()
{
#ifdef POSIX
	char buf[256];
	byte nb = 0;
	byte b;

	int c;
#endif

	debug("init");
	machine_init(&m);

#ifndef POSIX
	output("breakslights");
#endif

	debug("loop");

	for (;;) {
		if (missed > 0) {
			m.missed += missed;
			missed = 0;
		}
#ifdef POSIX
		c = getchar();
		if (c == EOF)
			break;

		b = c;

		if (b > 0) {
			buf[nb] = b;
			nb++;
		}

		if (b == '\n') {
			buf[nb] = '\0';
			handle_line(&m, buf);
			nb = 0;
		}
#else
		switch (u.state) {
		case U_IDLE:
			break;

		case U_ERROR:
			uart_puts("error\r\n");
			uart_clear(&u);
			break;

		case U_READY:
			_delay_ms(UART_WAIT);
			/* protect buffer from modification by interrupt */
			cli();

			if (handle_line(&m, u.buffer) == -1) {
				output("1");
			} else {
				output("0");
			}

			uart_clear(&u);

			sei();
			uart_cts(true);
			break;

		default:
			uart_puts("error\r\n");
		}
#endif

		if (draw) {
#ifndef POSIX
			uart_cts(false);
			_delay_ms(UART_WAIT);

			cli();
			machine_flush(&m);
			sei();

			uart_cts(true);
#else
			machine_flush(&m);
#endif
			machine_tick(&m);
			draw = false;
		}
	}

	machine_destroy(&m);
}

int main()
{
	setup();
	loop();
	return 0;
}
