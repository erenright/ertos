#ifndef _STRING_H
#define _STRING_H

#include <types.h>

void memcpy(void *dest, const void *src, size_t n);
void memmove(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
int strcmp(const char *s1, const char *s2);
int strlen(const char *s);

int atoi(const char *str);

#endif // !_STRING_H
