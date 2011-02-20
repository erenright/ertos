/*
 * list.c
 *
 * Lists and other similar functionality.
 */

#include <sys/list.h>
#include <sys/mem.h>

#include <types.h>

struct bfifo * bfifo_alloc(int nelems)
{
	struct bfifo *f;
	int fsize = sizeof(struct bfifo) + (sizeof(void *) * (nelems - 1));

	f = malloc(fsize);
	if (f == NULL)
		return NULL;

	memset(f, 0, fsize);

	f->size = nelems;
	f->free = nelems;

	return f;
}

int bfifo_queue(struct bfifo *f, void *elem)
{
	// No slots free
	if (f->free <= 0)
		return -1;

	f->elems[f->tail] = elem;

	// Bump the tail, accounting for wraparound
	if (++f->tail >= f->size)
		f->tail = 0;

	--f->free;

	return 0;
}

void * bfifo_dequeue(struct bfifo *f)
{
	void *elem;

	// No data
	if (f->free >= f->size)
		return NULL;

	elem = f->elems[f->head];

	// Bump the head, accounting for wraparound
	if (++f->head >= f->size)
		f->head = 0;

	++f->free;

	return elem;
}
