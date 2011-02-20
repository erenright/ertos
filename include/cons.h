#ifndef _CONS_H
#define _CONS_H

#include <types.h>
#include <sleep.h>

extern struct completion *cons_in_completion;

int cons_read(void *buf, size_t len);		// Read data
int cons_write(const void *buf, size_t len);	// Write data

#endif // !_CONS_H
