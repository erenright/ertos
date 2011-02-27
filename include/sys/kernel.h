#ifndef _KERNEL_H
#define _KERNEL_H

#include <types.h>
#include <proc.h>

#define HZ 100

// Must match up with HZ
#define clkticks_to_ms(x) (x * 10)
#define ms_to_clkticks(x) ((unsigned)x / 10)

extern struct kstat kstat;

// The number of times the timer interrupt has ticked
extern uint32_t clkticks;

extern struct self *kernel_self;

void arch_reset(void);

#endif // !_KERNEL_H
