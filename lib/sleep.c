/*
 * lib/sleep.c
 *
 * System calls for sleep-related functions.
 */

#include <sys/kernel.h>

#include <sleep.h>
#include <syscall.h>

// Returns 0 on success, -1 on error
int wait(struct completion *c)
{
	return __syscall(SYS_WAIT, (uint32_t)c);
}

int wake(struct completion *c)
{
	return __syscall(SYS_WAKE, (uint32_t)c);
}

// Sleep for period ms.
int sleep(uint32_t period)
{
	// This would ideally be done in the syscall itself, however
	// emulated division will be better off in user mode
	period = ms_to_clkticks(period);
	return __syscall(SYS_SLEEP, period);
}

int yield(void)
{
	return _syscall(SYS_YIELD);
}

int event_set(uint32_t mask)
{
	return __syscall(SYS_EVENT_SET, mask);
}

int event_wait(uint32_t mask)
{
	return __syscall(SYS_EVENT_WAIT, mask);
}

int alarm(struct alarm *a)
{
	return __syscall(SYS_ALARM, (uint32_t)a);
}

int _user_timer_trampoline_done(void)
{
	return _syscall(SYS_UTT_DONE);
}
