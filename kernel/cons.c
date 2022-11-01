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
 *   this list of conditions and the following disclaimer.
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
