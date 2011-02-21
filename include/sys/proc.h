#ifndef _PROC_H
#define _PROC_H

#include <sys/list.h>
#include <types.h>

enum proc_state {
	PROC_ACTIVE = 0,			// Currently running
	PROC_RUN,				// Wants to run
	PROC_SLEEP,				// Is waiting on something
	PROC_KILLED,				// Has been killed
};

struct proc {
	struct list	list;			// For scheduling

	int		pid;			// PID
	enum proc_state	state;			// Current state

	// WARNING: irq.s context switching depends on this offset!
	uint32_t	regs[17];		// Register set
	uint32_t	backup_regs[17];	// Backup register set

	uint32_t	*stack;			// Current stack
	void		*stack_base;		// Stack base address

	char		name[16];		// Name

	uint32_t	ticks_wakeup;		// Number of timer ticks
						// at which to force runnable

	uint32_t	event_mask;		// Event(s) that this proc
						// is waiting on

	struct {
		void (*handler)(void);
		uint32_t next;
		uint32_t period;
		int oneshot;
		int done;
		int active;
	} timer;
};

// kernel/sched.c
struct proc * spawn(void (*entry)(void), const char *name);

#endif // !_PROC_H
