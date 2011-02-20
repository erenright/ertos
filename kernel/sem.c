/*
 * sem.c
 *
 * Kernel-side semaphore implementation.
 */

#include <sys/sched.h>

#include <sem.h>
#include <string.h>
#include <stdio.h> // @@@ REMOVE ME when no longer needed

#define SEM_WAIT_SIZE	10	// Max 10 procs waiting on a semaphore

// arch/sem.s
void _sem_down(sem_t *sem);
void _sem_up(sem_t *sem);

void sem_init(sem_t *sem, int cur, int max, const char *id)
{
	memset(sem, 0, sizeof(sem_t));

	sem->cur = cur;
	sem->max = max;
	sem->wait = NULL;

	strncpy(sem->id, id, sizeof(sem->id) - 1);
}

void sem_free(sem_t *sem)
{
	puts("WARNING: sem_free not implemented");
}

void sem_down(sem_t *sem)
{
	// Can we lock the semaphore?
	if (sem_try_down(sem)) {
		// No, enter the wait queue
		// @@@ lock here!
		if (sem->wait)
			list_add_after(sem->wait, (struct list *)cur);
		else
			sem->wait = (struct list *)cur;
	}
}

