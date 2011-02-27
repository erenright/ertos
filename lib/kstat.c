/*
 * lib/kstat.c
 *
 * Kernel/system statistics and information.
 */

#include <kstat.h>
#include <syscall.h>

int kstat_get(struct kstat *ptr)
{
	return __syscall(SYS_KSTAT, (uint32_t)ptr);
}
