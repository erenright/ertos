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

#ifndef _DEV_UART_H
#define _DEV_UART_H

#include <types.h>
#include <sleep.h>

#define UART_FIFO_SIZE	1024

struct uart;

struct uart_ops {
	int (*open)(struct uart *);		// Open UART
	void (*close)(struct uart *);		// Close UART
	int (*set_baud)(struct uart *, int);	// Set BAUD rate
	void (*disable_tx)(struct uart *);	// Disable transmit
	void (*enable_tx)(struct uart *);	// Enable transmit
	void (*disable_rx)(struct uart *);	// Disable receive
	void (*enable_rx)(struct uart *);	// Enable receive
	void (*rx)(struct uart *);		// Receive handler
	void (*tx)(struct uart *);		// Transmit handler
};

struct uart_fifo {
	char data[UART_FIFO_SIZE];
	int head;
	int tail;
};

enum uart_state {
	UART_OPEN = 0,
	UART_CLOSED,
};

struct uart {
	struct uart_ops *uart_ops;	// Device I/O operations

	int baud;			// Current BAUD rate

	struct uart_fifo rx_fifo;	// Receive FIFO
	struct uart_fifo tx_fifo;	// Transmit FIFO

	enum uart_state state;		// Current state

	struct completion wait;		// Wait queue
};

// Returns the number of bytes free in the FIFO
static inline int uart_fifo_free(struct uart_fifo *f)
{
	if (f->tail > f->head)
		return f->tail - f->head;

	// Handle wraparound
	return sizeof(f->data) - (f->head - f->tail);
}

// Returns the number of bytes containted in the FIFO
static inline int uart_fifo_available(struct uart_fifo *f)
{
	if (f->tail >= f->head)
		return f->tail - f->head;

	// Handle wraparound
	return sizeof(f->data) - (f->head - f->tail);
}

int uart_read(struct uart *, void *buf, size_t len);		// Read data
int uart_write(struct uart *, const void *buf, size_t len);	// Write data

#endif // !_DEV_UART_H
