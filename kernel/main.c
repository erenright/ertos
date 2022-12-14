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
 * kernel/main.c
 *
 * Main kernel entry point. This module is required to call architecture,
 * memory, scheduling, and other core init functions before passing control off
 * to user tasks.
 */

#include <sys/irq.h>
#include <sys/sched.h>
#include <sys/kernel.h>

#include <stdio.h>
#include <kstat.h>

#include "../user/processes.h"

struct kstat kstat;

// arch/init.c
void arch_init(void);

// arch/eth.c
void ep9301_eth_init(void);

// net/dll/eth.c
int eth_init(void);

void run_boot_processes(void)
{
	struct proc *kproc;
	struct user_process *uproc;
	int i;

	// For each process..
	for (i = 0; boot_processes[i].main != NULL; ++i) {
		uproc = &boot_processes[i];

		kproc = spawn(uproc->main, uproc->name, PROC_USER);
		if (kproc)
			printf("spawned task \"%s\": %x @ %x (p %x, sb %x, s %x)\r\n",
				uproc->name,
				kproc->pid,
				(uint32_t)uproc->main,
				kproc,
				kproc->stack_base,
				kproc->self);
		else
			printf("failed to spawn task: %s\r\n", uproc->name);
	}
}

static char kstdout[STDOUT_SIZE];

struct self _kernel_self = {
	.stdout.ptr = kstdout,
	.stdout.idx = 0,
	.stdout.buf_enable = 1,
	.stdout.buf_last = 1
};

struct self *kernel_self = &_kernel_self;

struct mem_desc {
	int	least_remaining;// Least number of remaining chunks ever seen
	int	size;		// Chunk size
	struct bfifo *allocations;	// Allocated memory regions
};

extern struct mem_desc *mem_desc[];

// kernel/sched.c
void idle(void);

// Main kernel entry point. Perform initialization here.
void main(void)
{
	// Interrupts are disabled upon entry

	mem_init();	// Must come first in case arch and sched need malloc
	arch_init();
	sched_init();

	memset(&kstat, 0, sizeof(kstat));

	// OK, everything should be good to go so bring up the interrupts
	sti();

	puts("\r\n*** ERTOS - Eric's Real-Time Operating System ***\r\n");
	puts("Core arch and interrupts online");
	printf("Heap is 0x%x+0x%x\r\n",
		_heap_start, _heap_size);
	printf("Kernel self is 0x%x\r\n", kernel_self);

#ifdef CONFIG_NET
	ep9301_eth_init();
	eth_init();
#endif

	run_boot_processes();

	// Initial procs are ready, enable the scheduler
	enable_scheduler();

	// Enter the idle task.  The scheduler will take care of everything
	// from here.
	idle();
}
