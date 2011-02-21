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

void handle_task_timer_done(struct proc *p)
{
	// Restore regs
	memcpy(p->regs, p->backup_regs, sizeof(p->regs));

	p->timer.active = 0;
}

void handle_task_timer(struct proc *p)
{
	// user/timers.c
	extern void user_timer_trampoline(void);

	// Save regs for later restore
	memcpy(p->backup_regs, p->regs, sizeof(p->regs));

	// Swap in the handler routine
	// @@@ arch specific!
	p->regs[1] = (uint32_t)user_timer_trampoline;		// ??
	p->regs[2] = (uint32_t)p->timer.handler;		// r0
	p->regs[16] = (uint32_t)user_timer_trampoline;		// lr

	p->timer.done = 0;
	p->timer.active = 1;

	// Reset timer
	if (p->timer.oneshot)
		p->timer.next = 0;
	else
		p->timer.next = clkticks + p->timer.period;
}

