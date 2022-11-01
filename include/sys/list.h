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
 *   this list of conditions and the following disclaimer.
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
 * include/sys/list.h
 *
 * Simple lists and other similar collections.
 */

#ifndef _LIST_H
#define _LIST_H

#include <sys/mem.h>
#include <types.h>
#include <string.h>

struct list {
	struct list *next;
	struct list *prev;
};

static inline void list_add_after(void *list, void *add)
{
	struct list *l = (struct list *)list;
	struct list *a = (struct list *)add;
	
	a->next = l->next;
	a->prev = l;
	l->next = a;
}

static inline void list_remove(void *rem)
{
	struct list *l = (struct list *)rem;

	if (l->prev != NULL)
		l->prev->next = l->next;
	if (l->next != NULL)
		l->next->prev = l->prev;
}

#define list_empty(l) ((l)->next == NULL)

// A "bounded" list
struct bfifo {
	int	head;
	int	tail;
	int	free;
	int	size;
	void	*elems[1];
};

struct bfifo * bfifo_alloc(int nelems);
int bfifo_queue(struct bfifo *f, void *elem);
void * bfifo_dequeue(struct bfifo *f);

static inline void bfifo_free(struct bfifo *f)
{
	free(f);
}

#endif // !_LIST_H
