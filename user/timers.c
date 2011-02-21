/*
 * user/timers.c
 *
 * Support for user task timers.
 */

#include <sleep.h>

// This function is set as the task's new execution point by the scheduler
// if it's timer has expired.  The handler function is passed as |handler|
// and, after executing it, this function informs the kernel it has
// completed and may allow the task to resume normal execution.
void user_timer_trampoline(void (*handler)(void))
{
	handler();
	_user_timer_trampoline_done();
	while (1);
}
