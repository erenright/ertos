#ifndef _KERNEL_H
#define _KERNEL_H

#include <types.h>

#define HZ 100

// Must match up with HZ
#define clkticks_to_ms(x) (x * 10)
#define ms_to_clkticks(x) (x / 10)	// @@@ need div handlers

// The number of times the timer interrupt has ticked
extern uint32_t clkticks;

#endif // !_KERNEL_H
