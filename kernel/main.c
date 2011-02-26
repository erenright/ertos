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

#include "../user/processes.h"

struct kstat kstat;

// arch/init.c
void arch_init(void);

static void run_boot_processes(void)
{
	struct proc *kproc;
	struct user_process *uproc;
	int i;

	// For each process..
	for (i = 0; boot_processes[i].main != NULL; ++i) {
		uproc = &boot_processes[i];

		kproc = spawn(uproc->main, uproc->name);
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

// Main kernel entry point. Perform initialization here.
int main(void)
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

	run_boot_processes();

	// Initial procs are ready, enable the scheduler
	enable_scheduler();

	while (1) {
		// This is reached only very briefly. Once the first interrupt
		// occurs, the system idle task will replace this if there is
		// no task to schedule.
	}
}
