/*
 * kernel/main.c
 *
 * Main kernel entry point. This module is required to call architecture,
 * memory, scheduling, and other core init functions before passing control off
 * to user tasks.
 */
#include <sys/irq.h>
#include <sys/mem.h>
#include <sys/proc.h>
#include <sys/sched.h>

#include <types.h>
#include <string.h>
#include <stdio.h>
#include <sem.h>
#include <cons.h>

#include "../arch/regs.h"

// arch/init.c
void arch_init(void);



sem_t mysem;
static int print_null = 0;

void red_task(void)
{
	volatile uint8_t *leds = (uint8_t *)LEDS;

	*leds &= ~LED_RED;

	while (1) {
		// Sleep 1 second
		sleep(100);
		//event_wait(0x80);

		*leds ^= LED_RED;

		if (print_null) {
			int i = *(int *)NULL;

			printf("*NULL = %x\r\n", i);
		}
	}
}

static void print_procs(void)
{
	struct proc *p;
	struct proc *op;

	// @@@ dangerous, no locking
	p = cur;
	op = p;

	do {
		printf("0x%x\t", p->pid);

		switch (p->state) {
		case PROC_ACTIVE:
			printf("ACTIVE\t");
			break;

		case PROC_RUN:
			printf("RUN\t");
			break;

		case PROC_SLEEP:
			printf("SLEEP\t");
			break;

		case PROC_KILLED:
			printf("KILLED\t");
			break;

		default:
			printf("UNKNOWN\t");
			break;
		}

		printf("%s\r\n", p->name);

		p = (struct proc *)p->list.next;
	} while (p != op);
}

void ticker_task(void)
{
	static char ticker[] = { '/' , '-', '\\', '|' };	
	int tick = 0;
	char buf[128];
	int len = 0;

	while (1) {
		sleep(25); // 250ms

		putchar('\r');
		putchar(ticker[tick]);
		if (++tick >= sizeof(ticker))
			tick = 0;

		// Data read?
		len = cons_read(buf, sizeof(buf));
		if (len > 0)
			break;	// Yes, break to command prompt
	}

	putchar('\r');

	while (1) {
		printf("> ");

		if (NULL == gets(buf, sizeof(buf))) {
			puts("error reading string");
		} else {
			//event_set(0x80);
			puts("");

			// Ignore an empty string
			if (buf[0] == '\0')
				continue;

			// Command dispatch
			if (!strcmp(buf, "ps"))
				print_procs();
			else if (!strcmp(buf, "null"))
				print_null = 1;
			else
				printf("Unknown command \"%s\"\r\n", buf);
		}
	}
}

// Main kernel entry point. Perform initialization here.
int main(void)
{
	// Interrupts are disabled upon entry

	struct proc *ticker, *red;

	mem_init();	// Must come first in case arch and sched need malloc
	arch_init();
	sched_init();

	// OK, everything should be good to go so bring up the interrupts
	sti();

	puts("\r\n*** ERTOS - Eric's Real-Time Operating System ***\r\n");
	puts("Core arch and interrupts online");
	printf("Heap is 0x%x+0x%x\r\n",
		_heap_start, _heap_size);

	// @@@
	sem_init(&mysem, 1, 1, "mysem");

	red = spawn(red_task, "red");
	if (red)
		printf("spawned red task: %x @ %x (p %x, sb %x)\r\n",
			red->pid, (uint32_t)red_task, red, red->stack_base);
	else
		puts("failed to spawn red task");

	ticker = spawn(ticker_task, "ticker");
	if (ticker)
		printf("spawned ticker task: %x @ %x (p %x, sb %x)\r\n",
			ticker->pid, (uint32_t)ticker_task, ticker, ticker->stack_base);
	else
		puts("failed to spawn ticker task");

	// Initial procs are ready, enable the scheduler
	enable_scheduler();

	while (1) {
		// This is reached only very briefly. Once the first interrupt
		// occurs, the system idle task will replace this if there is
		// no task to schedule.
	}

}
