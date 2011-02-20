/*
 * user/console.c
 *
 * Console with user commands.  This task displays a ticker on the console,
 * waiting for the first user keystroke.  Upon receipt of the keystroke,
 * it drops to a command prompt.
 */

#include <sys/proc.h>
#include <sys/sched.h>

#include <stdio.h>
#include <sleep.h>
#include <cons.h>

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

void console_task(void)
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
			else
				printf("Unknown command \"%s\"\r\n", buf);
		}
	}
}

