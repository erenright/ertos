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
	while (n) {
		*(char *)dest++ = *(char *)src++;
		--n;
	}
}

void memmove(void *dest, const void *src, size_t n)
{
	char *d = (char *)dest;
	char *s = (char *)src;

	// If dest is before src, we can use memcpy
	if (dest < src)
		return memcpy(dest, src, n);

	// Otherwise we must perform a reverse memcpy
	s += n - 1;
	d += n - 1;
	while (n) {
		*d-- = *s--;
		--n;
	}
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
