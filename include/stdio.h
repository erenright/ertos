#ifndef _STDIO_H
#define _STDIO_H

// Stolen from host libc
#include <stdarg.h>

void puts(const char *s);
void putchar(char c);
int printf(char *fmt, ...);
char *gets(char *s, int size);

#endif // !_STDIO_H
