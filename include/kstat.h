/*
 * include/kstat.h
 *
 * Kernel/system statistics information.
 */

#ifndef _KSTAT_H
#define _KSTAT_H

#include <types.h>
#include "../net/dll/eth.h"

struct netstat {
	struct {
		char name[EN_ETH_IF_NAMESIZE];
		struct en_eth_stats stats;
	} eth;
};

struct kstat {
	uint32_t isr_recursion;
};

// syscall
int kstat_get(struct kstat *);
int netstat_get(struct netstat *);

#endif /* !_KSTAT_H */
