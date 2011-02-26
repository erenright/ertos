#ifndef _SCHED_H
#define _SCHED_H

#include <sys/proc.h>
#include <proc.h>

// kernel/sched.c
extern struct proc *cur;
extern struct self *self;

void sched_init(void);
void schedule(void);
void request_schedule(void);
void enable_scheduler(void);
void disable_scheduler(void);


#endif // !_SCHED_H
