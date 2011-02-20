/*
 * kernel/main.c
 *
 * Main kernel entry point. This module is required to call architecture,
 * memory, scheduling, and other core init functions before passing control off
 * to user tasks.
 */

#include <sys/irq.h>
#include <sys/sched.h>

#include <stdio.h>

#include "../user/processes.h"

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
			printf("spawned task \"%s\": %x @ %x (p %x, sb %x)\r\n",
				uproc->name,
				kproc->pid,
				(uint32_t)uproc->main,
				kproc,
				kproc->stack_base);
		else
			printf("failed to spawn task: %s\r\n", uproc->name);
	}
}

// Main kernel entry point. Perform initialization here.
int main(void)
{
	// Interrupts are disabled upon entry


	mem_init();	// Must come first in case arch and sched need malloc
	arch_init();
	sched_init();

	// OK, everything should be good to go so bring up the interrupts
	sti();

	puts("\r\n*** ERTOS - Eric's Real-Time Operating System ***\r\n");
	puts("Core arch and interrupts online");
	printf("Heap is 0x%x+0x%x\r\n",
		_heap_start, _heap_size);

	run_boot_processes();

	// Initial procs are ready, enable the scheduler
	enable_scheduler();

	while (1) {
		// This is reached only very briefly. Once the first interrupt
		// occurs, the system idle task will replace this if there is
		// no task to schedule.
	}
}
