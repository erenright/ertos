#ifndef _SEM_H
#define _SEM_H

#include <sys/list.h>

typedef struct {
	int	cur;	// Current semaphore value
	int 	max;	// Maximum number of locks

	char 	id[16];	// Textual ID

	struct	list *wait;
} sem_t;

void sem_init(sem_t *sem, int cur, int max, const char *id);
void sem_down(sem_t *sem);
void sem_free(sem_t *sem);

// arch/sem.c
int sem_try_down(sem_t *sem);
void sem_up(sem_t *sem);

#endif
