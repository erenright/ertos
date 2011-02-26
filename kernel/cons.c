/*
 * cons.c
 *
 * Basic console driver using a serial port.
 *
 * @@@ these should be system calls
 */

#include <sys/uart.h>
#include <sleep.h>
#include <string.h>

static struct uart *cons_uart = NULL;
struct completion *cons_in_completion;

void cons_init(struct uart *uart)
{
	cons_uart = uart;
	cons_in_completion = &uart->wait;
}

int cons_read(void *buf, size_t len)
{
	// If we have no console, return error
	if (cons_uart == NULL)
		return -1;

	return uart_read(cons_uart, buf, len);
}

int cons_write(const void *buf, size_t len)
{
	int n = 0;
	int rc;

	// If we have no console, return error
	if (cons_uart == NULL)
		return -1;

	do {
		rc = uart_write(cons_uart, buf + n, len - n);
		n += rc;
	} while (n < len && rc >= 0);

	return rc >= 0 ? n : rc;
}
