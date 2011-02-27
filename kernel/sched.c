#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/mem.h>
#include <sys/irq.h>
#include <sys/timers.h>

#include <cons.h>
#include <string.h>
#include <types.h>
#include <stdio.h>
#include <sleep.h>
#include <proc.h>

#define PROC_STACK_SIZE	4096	// 4k stack

// arch/cpu.s
void user_cpu_idle(void);

// @@@ use a hash table
static struct proc *procs = NULL;
static struct proc *idle_task = NULL;
struct proc *cur = NULL;
static int next_pid = 0;

struct self *self;

uint32_t _need_reschedule = 0;
int sched_enabled = 0;

// Returns non-zero on error
static int init_self(struct self *s)
{
	s->stdout.ptr = malloc(STDOUT_SIZE);
	if (s->stdout.ptr == NULL)
		return 1;

	s->stdout.idx = 0;
	s->stdout.buf_enable = 1;
	s->stdout.buf_last = 1;

	return 0;
}

struct proc * do_spawn(void (*entry)(void), const char *name)
{
	struct proc *proc = NULL;
	uint32_t *rptr;
	uint32_t spsr = 0x10; // USR mode @@@ arch specific!

	// @@@ shouldn't mix memory regions, proc stucts and proc stacks
	proc = malloc(sizeof(struct proc));
	if (proc != NULL) {
		memset(proc, 0, sizeof(struct proc));

		// @@@ this will overflow eventually
		proc->pid = ++next_pid;
		proc->state = PROC_RUN;
		proc->timer.ticks_wakeup = 0xFFFFFFFF;
		strncpy(proc->name, name, sizeof(proc->name));

		// Allocate a stack
		proc->stack_base = malloc(PROC_STACK_SIZE);
		if (proc->stack_base == NULL)
			goto out_err;

		// Stacks grow down @@@ this is arch-specific!
		proc->stack = proc->stack_base + PROC_STACK_SIZE - 4;

		// Load regs for later context switch
		rptr = proc->regs;
		#define SP *rptr++

		//SP = spsr;		// spsr
		SP = spsr;		// spsr
		SP = (uint32_t)entry;	// r14_IRQ ????
		SP = 0;			// r0
		SP = 0;			// r1
		SP = 0;			// r2
		SP = 0;			// r3
		SP = 0;			// r4
		SP = 0;			// r5
		SP = 0;			// r6
		SP = 0;			// r7
		SP = 0;			// r8
		SP = 0;			// r9
		SP = 0;			// r10
		SP = 0;			// r11
		SP = 0;			// r12
		SP = (uint32_t)proc->stack + PROC_STACK_SIZE - 4;
					// r13 / sp
		SP = (uint32_t)entry;	// r14 / lr
		#undef SP

		// Initialize "Self"
		proc->self = malloc(sizeof(struct self));
		if (proc->self == NULL)
			goto out_err;
		if (init_self(proc->self))
			goto out_err;

		// Add to process list
		proc->list.next = (struct list *)proc;
		proc->list.prev = (struct list *)proc;
	}

	return proc;

out_err:

	if (proc != NULL) {
		if (proc->stack_base != NULL)
			free(proc->stack_base);

		if (proc->self != NULL)
			free(proc->self);

		free(proc);
	}

	return NULL;
}

struct proc * spawn(void (*entry)(void), const char *name)
{
	struct proc *proc;

	proc = do_spawn(entry, name);
	if (proc != NULL) {
		cli(); // @@@ spin lock, save irq mask, etc
		if (procs == NULL)
			procs = proc;
		else
			list_add_after(procs, proc);
		sti();
	}

	return proc;
}

void request_schedule(void)
{
	if (sched_enabled)
		_need_reschedule = 1;
}

void enable_scheduler(void)
{
	sched_enabled = 1;
}

void disable_scheduler(void)
{
	sched_enabled = 0;
}

// Determine if this task has any timers coming up.
// If so, it may join the run queue
static void check_timers(struct proc *p)
{
	// Does this task have an associated timer that is not running?
	if (p->timer.next > 0 && !p->timer.active) {
		// Yes, has it tripped?
		if (clkticks >= p->timer.next) {
			// Yes, handle it and place on run queue
			handle_task_timer(p);

			// This task is now eligible to run
			goto out;
		}
	}
	// Does this task have an associated timer that is active but done?
	else if (p->timer.active && p->timer.done) {
		// Yes, restore the previous context
		handle_task_timer_done(p);
	}

	// Is this task sleeping?
	if (p->state == PROC_SLEEP) {
		// Yes, can it be woken up?
		if (clkticks >= p->timer.ticks_wakeup) {
			// Yes, select it
			p->timer.ticks_wakeup = 0xFFFFFFFF;
			p->state = PROC_RUN;
			goto out;
		}
	}

out:
	return;
}

static void swtch(struct proc *next)
{
	next->state = PROC_ACTIVE;

	cur = next;
	self = cur->self;
}

// Do not call printf from this function
void schedule(void)
{
	struct proc *p, *op;
	struct proc *next;

	// Assume we find nothing
	next = idle_task;

	if (procs == NULL)
		goto out;

	// No current task?
	if (cur == NULL) {
		// Start at the top
		p = procs;
	} else {
		// Is the current task the idle task?
		if (cur != idle_task) {
			// No, place cur on the run queue if it is still
			// active
			if (cur->state == PROC_ACTIVE)
				cur->state = PROC_RUN;

			// Select the next task beyond cur
			p = (struct proc *)cur->list.next;
		} else {
			// Yes, the current task is the idle task. Put it to
			// sleep and select from the list head
			cur->state = PROC_SLEEP;
			p = procs;
		}
	}

	// Find a task who is not waiting
	op = p;
	do {
		// Determine if this task has any timers which have fired
		check_timers(p);

		// Is this task on the run queue?
		if (p->state == PROC_RUN) {
			// Yes, select it
			next = p;
			break;
		}

		p = (struct proc *)p->list.next;
	} while (p != op);

out:

	swtch(next);

	_need_reschedule = 0;
}

// Idle task
void idle(void)
{
	//static volatile uint8_t *leds = (uint8_t *)0x80840020;
	//int c = 0;

	while (1) {
		/*if (++c >= 100000) {
			c = 0;
			*leds ^= 0x03;
		}*/
		//*leds ^= 0x02;
		yield();
	}
}

void sched_init(void)
{
	const char sched_init_err[] = "sched_init: failed to create idle task\r\n";

	self = kernel_self;

	//idle_task = do_spawn(idle, PROC_SVC);
	idle_task = do_spawn(idle, "[idle]");
        idle_task->state = PROC_SLEEP;
	if (idle_task == NULL) {
		cons_write(sched_init_err, sizeof(sched_init_err));
		while (1);	// @@@ panic()
	}

	// Required for early printf
	cur = idle_task;
}

