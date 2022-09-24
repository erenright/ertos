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

#include <types.h>

void *memset(void *s, int c, size_t n)
{
	void *r = s;
	while (n != 0) {
		*(char *)s++ = c & 0xff;
		--n;
	}

	return r;
}

void memcpy(void *dest, const void *src, size_t n)
{
	if (dest == src)
		return;

	while (n--)
		*(char *)dest++ = *(char *)src++;
}

void memmove(void *dest, const void *src, size_t n)
{
	if (dest == src)
		return;

	// If dest is before src, we can use memcpy
	if (dest < src)
		return memcpy(dest, src, n);

	// Otherwise we must perform a reverse memcpy
	src += n - 1;
	dest += n - 1;
	while (n--)
		*(char *)dest-- = *(char *)src--;
}

char *strcpy(char *dest, const char *src)
{
	char *r = dest;

	do {
		*dest++ = *src;
	} while (*src++ != '\0');

	return r;
}

char *strncpy(char *dest, const char *src, size_t n)
{
	char *r = dest;

	do {
		*dest++ = *src;
	} while (*src++ != '\0' && --n > 0);

	// Pad remaining bytes with \0
	while (n-- > 0)
		*dest++ = '\0';

	return r;
}

int strcmp(const char *s1, const char *s2)
{
	do {
		if (*s1 < *s2)
			return -1;
		else if (*s1 > *s2)
			return 1;

		++s1;
		++s2;
	} while (*s1 != '\0' && *s2 != '\0');

	// One string is not terminated
	if (*s1 || *s2)
		return 1;

	return 0;
}

int strlen(const char *s)
{
	int len = 0;

	while (*s++ != '\0')
		++len;

	return len;
}

int atoi(const char *str)
{
	const char *p = str;
	int val = 0;
	int neg = 0;

	if (*p == '-') {
		// Is this POSIX compliant?
		neg = 1;
		++p;
	}

	while (*p >= '0' && *p <= '9') {
		val *= 10;
		val += *p - '0';
		++p;
	}

	if (neg) {
		// Convert to two's complement if negative previously detected
		val = ~val;
		++val;
	}

	return val;
}
