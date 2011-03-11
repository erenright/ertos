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

	ep9301_eth_init();
	eth_init();

	run_boot_processes();

	// Initial procs are ready, enable the scheduler
	enable_scheduler();

	// Enter the idle task.  The scheduler will take care of everything
	// from here.
	idle();
}
