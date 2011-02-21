/*
 * include/sleep.h
 *
 * Sleep-related functions and definitions.
 */

#ifndef _SLEEP_H
#define _SLEEP_H

#include <sys/list.h>
#include <types.h>

struct completion {
	struct bfifo	*wait;	// Process waiting
};

struct alarm {
	int msec;		// Number of milliseconds to wait
	void (*handler)(void);	// Function handler
	int oneshot;		// Non-zero if this should fire only once
};

// Public system calls
int wait(struct completion *c);
int wake(struct completion *c);
int sleep(uint32_t period);
int yield(void);
int event_set(uint32_t mask);
int event_wait(uint32_t mask);
int alarm(struct alarm *a);

// Private system calls
int _user_timer_trampoline_done(void);

#endif // !_SLEEP_H
