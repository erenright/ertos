/*
 * user/processes.c
 *
 * Shared user process code and initialization structures.
 */

#include <types.h>
#include "processes.h"

// red.c
extern void red_task(void);

// console.c
extern void console_task(void);

// These processes are initialized at power-up in order
struct user_process boot_processes[] = {
	{ "red", red_task },
	{ "console", console_task },
	{ NULL, NULL }
};
