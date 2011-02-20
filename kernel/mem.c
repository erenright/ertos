/*
 * kernel/mem.c
 *
 * Implementation of simple memory allocator and other memory handling
 * functions.
 */

#include <sys/mem.h>
#include <sys/sched.h>

#include <stdio.h>

// These come in from the linker
extern void *__heap_start__;
extern void *__heap_end__;
extern void *__stack_start__;
extern void *__stack_end__;
extern void *__end;

void *_heap_start = NULL;
size_t _heap_size = 0;
static void *heap_cur = NULL;

void mem_init(void)
{
	// @@@ why does __end come out with the wrong value?
	// @@@ WARNING this restricts code+data+stack to ~1MB
	//_heap_start = __end;
	_heap_start = (void *)0x00200000;
	_heap_size = 0x00100000;
	heap_cur = _heap_start;
}

void * malloc(size_t size)
{
	void *p = NULL;

	// Do we have enough room to service the request?
	if (_heap_size - (heap_cur - _heap_start) >= size) {
		// @@@ watch for underflow of uint
		// Yes
		p = heap_cur;
		heap_cur += size;

		// Realign heap_cur
		if ((uint32_t)heap_cur % 4)
			heap_cur += 4 - ((uint32_t)heap_cur % 4);
	}
		
	//printf("allocated %x to %x (req %x)\r\n", p, heap_cur - 1, size);
	return p;
}

void free(void *ptr)
{
	// STUB
}

void user_page_fault(void *ptr)
{
	printf("%s: illegal pointer access at 0x%x, killing process\r\n",
		cur->name, ptr);

	// @@@ this should be a function somewhere
	cur->state = PROC_KILLED;
	cur->ticks_wakeup = 0xFFFFFFFF;
	cur->event_mask = 0;

	request_schedule();
}
