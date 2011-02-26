/*
 * dev/uart.c
 *
 * General device-agnostic UART interface.
 */

#include <sys/uart.h>

static inline void uart_fifo_add(struct uart_fifo *f,const void *buf,size_t len)
{
	char *p = (char *)buf;

	// @@@ room for enhancement here, interrupts are disabled and this
	// is slow!
	do {
		f->data[f->tail] = *p++;

		if (++f->tail >= sizeof(f->data))
			f->tail = 0;
	} while (--len > 0);
}

static inline void uart_fifo_remove(struct uart_fifo *f, void *buf, size_t len)
{
	char *p = (char *)buf;

	// @@@ room for enhancement here, interrupts are disabled and this
	// is slow!
	do {
		*p++ = f->data[f->head];

		if (++f->head >= sizeof(f->data))
			f->head = 0;
	} while (--len > 0);
}

int uart_write(struct uart *uart, const void *buf, size_t len)
{
	int rc = 0;	// Default to no data written
	int available;

	// Disable transmit so we don't clash with the transmit queue
	uart->uart_ops->disable_tx(uart);

	available = uart_fifo_free(&uart->tx_fifo);

	// Can we write any data?
	if (available > 0) {
		// Yes, how much?
		if (len > available)
			len = available;

		uart_fifo_add(&uart->tx_fifo, buf, len);

		rc = len;
	}

	// OK, data is ready to go
	uart->uart_ops->enable_tx(uart);

	return rc;
}

int uart_read(struct uart *uart, void *buf, size_t len)
{
	int rc = 0;	// Default to no data read
	int available;

	// Disable receive so we don't clash with the receive queue
	uart->uart_ops->disable_rx(uart);

	available = uart_fifo_available(&uart->rx_fifo);

	// Can we read any data?
	if (available > 0) {
		// Yes, how much?
		if (len > available)
			len = available;

		uart_fifo_remove(&uart->rx_fifo, buf, len);

		rc = len;
	}

	// OK, done reading to re-enable receipt
	uart->uart_ops->enable_rx(uart);

	return rc;
}
