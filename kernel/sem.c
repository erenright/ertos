/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2022, Eric Enright
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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

