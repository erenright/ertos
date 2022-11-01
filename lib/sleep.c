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

int reset(void)
{
	return _syscall(SYS_RESET);
}
