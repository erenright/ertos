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

#include <cons.h>
#include <sleep.h>
#include <proc.h>
#include <sys/sched.h>

// Stolen from host libc
#include <stdarg.h>

#define CTRL_U	0x15

// Set to true if input should echo characters
static int echo = 1;

void flush(void)
{
	cons_write(self->stdout.ptr, self->stdout.idx);
	self->stdout.idx = 0;
}

// Attempt to queue a char.  If the queue is full, flush it and
// then place the char.
void putchar(char c)
{
	if (self->stdout.buf_enable) {
		// Flush queue if required
		if (self->stdout.idx >= STDOUT_SIZE)
			flush();

		// Queue char
		self->stdout.ptr[self->stdout.idx++] = c;

		// Flush on newline
		if (c == '\n')
			flush();
	} else {
		cons_write(&c, 1);
	}
}

void puthexchar(char c)
{
	char x;
	if ((c >> 4) > 9)
		x = (c >> 4) - 10 + 'A';
	else
		x = (c >> 4) + '0';

	putchar(x);

	if ((c & 0xf) > 9)
		x = (c & 0xf) - 10 + 'A';
	else
		x = (c & 0xf) + '0';

	putchar(x);
}

static void _puts(const char *s)
{
	while (*s != '\0')
		putchar(*s++);
}

void puts(const char *s)
{
	_puts(s);
	putchar('\r');
	putchar('\n');
}

static int printf_d(int d)
{
	int num = 0;
	char buf[10];	// Max 11 chars in sint32_t (+1 for -)
	int i = 0;

	// Quick handling of zero
	if (d == 0) {
		putchar('0');
		return 1;
	}

	// Is this a negative number?
	if (d & 0x80000000) {
		// Yes, convert to positive and add '-' to output
		// @@@ implement abs() ?
		--d;
		d = ~d;
		putchar('-');
	}

	// Find out how many digits are in this number
	while (d > 0) {
		buf[i++] = (d % 10) + '0';
		d /= 10;
	}

	num = i;

	// Now reverse number the characters
	while (i > 0)
		putchar(buf[--i]);

	return num;
}

static int printf_x(uint32_t x)
{
	uint8_t c;
	int n = 0;

	c = (x & 0xFF000000) >> 24;
	if (c) {
		puthexchar(c);
		n += 2;
	}

	c = (x & 0x00FF0000) >> 16;
	if (c || n) {
		puthexchar(c);
		n += 2;
	}

	c = (x & 0x0000FF00) >> 8;
	if (c || n) {
		puthexchar(c);
		n += 2;
	}

	c = x & 0xFF;
	if (c || n) {
		puthexchar(c);
		n += 2;
	}

	if (!n) {
		putchar('0');
		++n;
	}

	return n;
}

// This return value can not be trusted
int printf(char *fmt, ...)
{
	va_list ap;
	int d;
	uint32_t x;
	const char *s;
	int chars = 0;

	va_start(ap, fmt);

	while (*fmt != '\0') {
		switch (*fmt) {
		case '%':	// Format requested
			switch (*++fmt) {
				case 'd':	// Decimal
					d = va_arg(ap, int);
					chars += printf_d(d);
					break;

				case 'x':
					x = va_arg(ap, uint32_t);
					chars += printf_x(x);
					break;

				case 's':
					s = va_arg(ap, const char *);
					// @@@ _puts does not return chars written
					_puts(s);
					break;

				case '%':	// Pass through
				default:	// Unknown format, pass though
					putchar(*fmt);
					break;
			}

			break;

		default:	// Unmolested character
			putchar(*fmt);
			chars += 1;
			break;
		}

		++fmt;
	}

	va_end(ap);

	return chars;
}

int getchar(void)
{
	char c;
	int status;

	do {
		status = cons_read(&c, 1);
		if (status < 0)
			return -1;

		if (status == 0) {
			wait(cons_in_completion);
			continue;
		}
	} while (status == 0);

	return c;
}

// A hybrid gets() / fgets()
char *gets(char *s, int size)
{
	char *rp = s;
	char *orig_s = s;
	char c;
	int status;

	stdio_buf_disable();

	do {
		// Read a byte
		status = cons_read(&c, 1);

		// Error
		if (status < 0) {
			rp = NULL;
			goto out;
		}

		// No character available, sleep
		if (status == 0) {
			wait(cons_in_completion);
			continue;
		}

		// Process char
		switch (c) {
		case '\r':
		case '\n':
			// Do nothing
			break;

		case '\b':	// Backspace
			if (s > orig_s) {
				_puts("\b \b");
				--s;
			}
			break;

		case CTRL_U:	// Wipe line
			while (s > orig_s) {
				_puts("\b \b");
				--s;
			}
			break;

		default:
			// Store char
			*s++ = c;
			--size;

			if (echo)
				putchar(c);
			break;
		}
	} while (c != '\n' && c != '\r' && size > 0);

	// Terminate string
	*s = '\0';

out:
	stdio_buf_restore();

	return rp;
}
