#include <stdbool.h>

#define UART_BUFFER_SIZE	128

enum uartstate {
	U_IDLE,
	U_READY,
	U_ERROR
};

struct uart {
	char buffer[UART_BUFFER_SIZE + 1];
	volatile unsigned char tail;
	volatile enum uartstate state;
};

void uart_init(struct uart *u);
void uart_clear(struct uart *u);
void uart_cts(bool value);
void uart_putchar(const char c);
void uart_puts(const char *s);
