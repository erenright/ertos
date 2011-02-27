/*
 * include/kstat.h
 *
 * Kernel/system statistics information.
 */

#ifndef _KSTAT_H
#define _KSTAT_H

#include <types.h>

struct kstat {
	uint32_t isr_recursion;
};

// syscall
int kstat_get(struct kstat *);

#endif /* !_KSTAT_H */
