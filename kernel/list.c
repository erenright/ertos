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
