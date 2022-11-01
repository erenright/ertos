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

#ifndef _SYS_PROC_H
#define _SYS_PROC_H

#include <sys/list.h>
#include <types.h>
#include <proc.h>

enum proc_state {
	PROC_ACTIVE = 0,			// Currently running
	PROC_RUN,				// Wants to run
	PROC_SLEEP,				// Is waiting on something
	PROC_KILLED,				// Has been killed
};

enum proc_mode {
	PROC_USER = 0,				// Normal user-space
	PROC_SYSTEM,				// Kernel/priviledged task
};

struct proc {
	struct list	list;			// For scheduling

	int		pid;			// PID
	enum proc_state	state;			// Current state

	// WARNING: irq.s context switching depends on this offset!
	uint32_t	regs[17];		// Register set
	uint32_t	backup_regs[17];	// Backup register set

	enum proc_mode	mode;			// Task mode

	uint32_t	*stack;			// Current stack
	void		*stack_base;		// Stack base address

	char		name[16];		// Name

	uint32_t	event_mask;		// Event(s) that this proc
						// is waiting on

	struct {
		void (*handler)(void);
		uint32_t next;
		uint32_t period;
		int oneshot;
		int done;
		int active;
		int last_state;

		uint32_t ticks_wakeup;		// Number of timer ticks
						// at which to force runnable

		uint32_t last_ticks_wakeup;
	} timer;

	struct self *self;
};

// kernel/sched.c
struct proc * spawn(void (*entry)(void), const char *name, enum proc_mode mode);

#endif // !_SYS_PROC_H
