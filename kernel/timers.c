/*
 * kernel/timers.c
 *
 * Various timer-related functions, including the main timer interrupt.
 */

#include <sys/sched.h>

#include <types.h>

uint32_t clkticks = 0;

// Main timer interrupt
void timer_int(void)
{
	// Bump the kernel clock
	++clkticks;

	// Schedule another user task
	request_schedule();
}
