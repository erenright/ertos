/*
 * include/sys/timers.h
 *
 * System timer-related items, such as the main timer interrupt and the overall
 * clock tick counter.
 */

#ifndef _SYS_TIMERS_H
#define _SYS_TIMERS_H

#include <types.h>
#include <sys/proc.h>

extern uint32_t clkticks;

void timer_int(void);
void handle_task_timer_done(struct proc *p);
void handle_task_timer(struct proc *p);

#endif // !_SYS_TIMERS_H
