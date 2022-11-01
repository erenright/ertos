/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2022, Eric Enright
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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

	p->state = p->timer.last_state;
	p->timer.ticks_wakeup = p->timer.last_ticks_wakeup;
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

	p->timer.last_state = p->state;
	p->timer.last_ticks_wakeup = p->timer.ticks_wakeup;
	p->state = PROC_RUN;
}

