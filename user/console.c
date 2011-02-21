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
#include <string.h>

// Maximum number of arguments a command may have
#define MAX_ARGS 8

//#define _DBG

static inline int isspace(int c)
{
	if (c == ' ')
		return 1;
	if (c == '\t')
		return 1;
	if (c == '\r')
		return 1;
	if (c == '\n')
		return 1;
	if (c == '\f')
		return 1;
	if (c == '\v')
		return 1;

	return 0;
}

static void cmd_ps(int argc, char *argv[])
{
	struct proc *p;
	struct proc *op;

	printf("PID\tState\tName\r\n");

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

static int ticker_done = 0;

static void cmd_ticker(int argc, char *argv[])
{
	static char ticker[] = { '/' , '-', '\\', '|' };	
	int tick = 0;
	int len = 0;
	char buf[128];

	ticker_done = 0;

	while (!ticker_done) {
		sleep(250);

		putchar('\r');
		putchar(ticker[tick]);
		if (++tick >= sizeof(ticker))
			tick = 0;

		// Data read?
		len = cons_read(buf, sizeof(buf));
		if (len > 0) {
			ticker_done = 1;// Yes, break to command prompt
		}
	}

	putchar('\r');
}

static void cmd_exit(int argc, char *argv[])
{
	ticker_done = 0;
}

static void cmd_dumpmem(int argc, char *argv[])
{
	//int i;
	int len;
	int addr;
	int x;

	if (argc != 3) {
		puts("dumpmem <addr> <len>");
		return;
	}

	addr = atoi(argv[1]);
	len = atoi(argv[2]);
	if (len == 0)
		len = 16;

	printf("dumping 0x%x+0x%x\r\n\r\n", addr, len);

	x = 0;
	while (len-- > 0) {
		if (x == 0)
			printf("%x| ", addr);
		else if (x == 4)
			printf("   ");

		printf("%x ", *(char *)addr);

		if (++x >= 8) {
			x = 0;
			puts("");
		}

		++addr;
	}

	puts("");
}

void handle_alarm(void)
{
	printf("alarm hit!\r\n");
}

static void cmd_alarm(int argc, char *argv[])
{
	struct alarm a;

	if (argc != 3) {
		printf("alarm <msec> <oneshot>\r\n");
		return;
	}

	a.msec = atoi(argv[1]);
	a.oneshot = atoi(argv[2]);
	a.handler = handle_alarm;

	alarm(&a);

	printf("alarm armed\r\n");
}

static void cmd_reset(int argc, char *argv[])
{
	reset();
	/*NOTREACHED*/

	printf("reset failed!?\r\n");
}

struct command {
	const char *name;
	void (*func)(int, char **);
	const char *description;
};

static void cmd_help(int, char **);

struct command commands[] = {
	{ "?", cmd_help, "synonym for \"help\"" },
	{ "alarm", cmd_alarm, "test alarm: alarm <msec> <oneshot>" },
	{ "dumpmem",cmd_dumpmem, "dump memory location: dumpmem <addr> <len>" },
	{ "exit", cmd_exit, "exit the console" },
	{ "help", cmd_help, "list available commands" },
	{ "ps", cmd_ps, "list running processes" },
	{ "reset", cmd_reset, "reset the system" },
	{ NULL, NULL }
};

static void cmd_help(int argc, char *argv[])
{
	struct command *cmd;
	int i;

	for (i = 0; commands[i].name != NULL; ++i) {
		cmd = &commands[i];

		printf("%s - %s\r\n", cmd->name, cmd->description);
	}
}

static void handle_command(int argc, char *argv[])
{
	struct command *cmd;
	int i;

	for (i = 0; commands[i].name != NULL; ++i) {
		cmd = &commands[i];

		// @@@ need strncmp
		if (!strcmp(cmd->name, argv[0])) {
			cmd->func(argc, argv);
			return;
		}
	}

	printf("unknown command: %s\r\n", argv[0]);
}

void console_task(void)
{
	char buf[128];
	int argc;
	char *argv[MAX_ARGS];
	char *p;
#ifdef _DBG
	int i;
#endif

	while (1) {
		if (!ticker_done)
			cmd_ticker(0, NULL);

		printf("> ");

		if (NULL == gets(buf, sizeof(buf))) {
			puts("error reading string");
		} else {
			puts("");

			// Ignore an empty string
			if (buf[0] == '\0')
				continue;

			// Break up the arguments
			argc = 0;
			memset(argv, 0, sizeof(char *) * MAX_ARGS);

			p = buf;
			while (*p != '\0') {
				// Find the first non-whitespace
				while (isspace(*p))
					*p++ = '\0';

				if (*p == '\0')
					break;

				argv[argc++] = p;

				// Find the next whitespace
				while (!isspace(*p) && *p != '\0')
					++p;
			}

			// We need at least one arg/cmd
			if (argc <= 0)
				continue;

#ifdef _DBG
			printf("argc is 0x%x\r\n", argc);
			for (i = 0; i < argc; ++i)
				printf("0x%x \"%s\"\r\n", i, argv[i]);
#endif

			// Command dispatch
			handle_command(argc, argv);
		}
	}
}

