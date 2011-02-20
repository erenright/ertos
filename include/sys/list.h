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
