/*
 * kernel/syscall.c
 *
 * Main system call implementation.
 */

#include <sys/kernel.h>
#include <sys/sched.h>
#include <sys/list.h>

#include <syscall.h>
#include <types.h>
#include <stdio.h>
#include <sleep.h>

static int sys_wait(uint32_t *args)
{
	struct completion *c = (struct completion *)*args;
	int rc = 0;

	if (c == NULL)
		return -1;

	// Add the process to the wait queue
	if (!bfifo_queue(c->wait, cur)) {
		// Success
		cur->state = PROC_SLEEP;
		request_schedule();
	} else {
		printf("sys_wait: failed to add proc to wait queue: %x\r\n",
			cur->pid);
		rc = -1;
	}

	return rc;
}

int sys_wake(uint32_t *args)
{
	struct completion *c = (struct completion *)*args;
	struct proc *proc;

	// Dequeue the entire wait queue and add each proc to the run list
	while (NULL != (proc = (struct proc *)bfifo_dequeue(c->wait))) {
		proc->state = PROC_RUN;
	}

	request_schedule();

	return 0;
}

// Period to sleep, in ms
static int sys_sleep(uint32_t *arg)
{
	uint32_t period = *arg;

	cur->state = PROC_SLEEP;
	cur->ticks_wakeup = clkticks + period;// @@@ must use ms_to_clkticks
	request_schedule();

	return 0;
}

static int sys_yield(uint32_t *arg)
{
	request_schedule();
	return 0;
}

static int sys_event_set(uint32_t *arg)
{
	uint32_t mask = *arg;
	struct proc *p;
	int hit = 0;

	// Walk the process list and wake up anyone waiting on this mask
	for (		p = (struct proc *)cur->list.next;
			p != cur;
			p = (struct proc *)p->list.next) {
		if (p->event_mask & mask) {
			hit = 1;
			p->state = PROC_RUN;
			p->event_mask &= ~mask;
		}
	}

	if (hit)
		request_schedule();

	return 0;
}

static int sys_event_wait(uint32_t *arg)
{
	uint32_t mask = *arg;

	cur->event_mask |= mask;
	cur->state = PROC_SLEEP;
	request_schedule();

	return 0;
}

static int sys_alarm(uint32_t *arg)
{
	struct alarm *a = (struct alarm *)*arg;

	cur->timer.handler = a->handler;
	cur->timer.period = ms_to_clkticks(a->msec);
	cur->timer.next = clkticks + cur->timer.period;
	cur->timer.oneshot = a->oneshot;

	return 0;
}

static int sys_utt_done(uint32_t *arg)
{
	cur->timer.done = 1;
	request_schedule();

	return 0;
}

static int sys_reset(uint32_t *arg)
{
	arch_reset();
	/*NOTREACHED*/

	return 0;
}

static void *syscall_table[] = {
	sys_wait,	// 0
	sys_wake,	// 1
	sys_sleep,	// 2
	sys_yield,	// 3
	sys_event_set,	// 4
	sys_event_wait,	// 5
	sys_alarm,	// 6
	sys_utt_done,	// 7
	sys_reset	// 8
};

int c_svc(uint32_t num, uint32_t *regs)
{
	uint32_t real_num = *regs;
	int (*func)(uint32_t *regs);
	int rc = -1;

	self = kernel_self;

	if (real_num > (sizeof(syscall_table) >> 2)) {
		printf("invalid syscall: 0x%x\r\n", real_num);
		rc = -1;
	} else {
		func = syscall_table[real_num];
		rc = func(regs + 1);
	}

	self = cur->self;

	return rc;
}

