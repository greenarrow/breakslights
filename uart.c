#include <stdbool.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/setbaud.h>

#include "uart.h"

#define CTS_DDR		DDRB
#define CTS_PORT	PORTB
#define CTS_PIN		PINB1

struct uart *tty = NULL;

ISR(USART_RX_vect)
{
	char c;
	c = UDR0;

	if (tty == NULL)
		return;

	/* FIXME: we need to put up to 3 extra characters somewhere
	 * as another line may have begun. The host currently sends lines
	 * synchronously to work around this. */

	if (tty->state == U_READY) {
		tty->state = U_ERROR;
		tty->tail = 0;
		return;
	}

	if(tty->tail + 2 > UART_BUFFER_SIZE) {
		tty->state = U_ERROR;
		tty->tail = 0;
		return;
	}

	tty->buffer[tty->tail] = c;

	if (tty->buffer[tty->tail] == '\n') {
		uart_cts(false);
		tty->buffer[tty->tail + 1] = '\0';
		tty->state = U_READY;
		return;
	}

	tty->tail++;
}

void uart_init(struct uart *u)
{
	tty = u;
	tty->tail = 0;
	tty->state = U_IDLE;

	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;

#if USE_2X
	UCSR0A |= _BV(U2X0);
#else
	UCSR0A &= ~(_BV(U2X0));
#endif

	/* Enable RX, TX & RC interrupt  */
	UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);

	/* 8-bit data */
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);

	/* Set pin for CTS output */
	CTS_DDR |= _BV(CTS_PIN);

	uart_cts(true);
}

void uart_clear(struct uart *u)
{
	u->tail = 0;
	u->state = U_IDLE;
}

void uart_cts(bool value)
{
	if (value)
		CTS_PORT &= ~_BV(CTS_PIN);
	else
		CTS_PORT |= _BV(CTS_PIN);
}

void uart_putchar(const char c)
{
	/* Wait until data register empty */
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;
}

void uart_puts(const char *s)
{
	size_t i;

	for (i = 0; i < strlen(s); i++)
		uart_putchar(s[i]);
}
