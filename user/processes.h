/*
 * user/processes.h
 *
 * User process definitions.
 */

#ifndef _USER_PROCESSES_H
#define _USER_PROCESSES_H

struct user_process {
	const char *name;
	void (*main)(void);
};

// Processes to initialize at power-up
extern struct user_process boot_processes[];

#endif /* !_USER_PROCESSES_H */
