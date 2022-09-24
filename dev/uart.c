/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2022, Eric Enright
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
