/*
 * kernel/mem.c
 *
 * Implementation of simple memory allocator and other memory handling
 * functions.
 */

#include <sys/mem.h>
#include <sys/sched.h>
#include <sys/list.h>

#include <stdio.h>

#define ALLOC_MIN	32	// Smallest allocation chunk
#define ALLOC_STEPS	9	// Number of allocation shifts, max chunk 8k
#define ALLOC_NUM	128	// Number of chunks per allocation step
#define SMALLOC_SIZE	0x00100000	// 1MB for smalloc

struct mem_desc {
	int	least_free;	// Least number of remaining chunks ever seen
	int	size;		// Chunk size
	void	*start;		// Allocation region start
	void	*end;		// Allocation region end
	struct bfifo *allocations;	// Allocated memory regions
};

// These come in from the linker
extern void *__heap_start__;
extern void *__heap_end__;
extern void *__stack_start__;
extern void *__stack_end__;
extern void *__end;

void *_heap_start = NULL;
size_t _heap_size = 0;
static void *heap_cur = NULL;

struct mem_desc *mem_desc[ALLOC_STEPS];

int dmalloc_enabled = 0;	// true of "dynamic" malloc was initialized

// Simple/static malloc used for bootstrap and permanently-allocated memory
void * smalloc(size_t size)
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
		
	return p;
}

void * malloc(size_t size)
{
	int i;
	void *p = NULL;

	// Fall back to simple malloc if required
	if (!dmalloc_enabled)
		return smalloc(size);

	// Find a suitable memory descriptor
	for (i = 0; i < ALLOC_STEPS; ++i) {
		// Is this step large enough?
		if (mem_desc[i]->size < size)
			continue;	// No

		// Yes. Is there room for us?
		// @@@ need locking here!!
		p = bfifo_dequeue(mem_desc[i]->allocations);
		if (p != NULL) {
			// Yes
			if (mem_desc[i]->allocations->free < mem_desc[i]->least_free)
				mem_desc[i]->least_free = mem_desc[i]->allocations->free;
			break;
		}
	}

	return p;
}

void free(void *ptr)
{
	int i;

	// Simple malloc can not free
	if (!dmalloc_enabled)
		return;

	// Determine which descriptor this pointer fits within
	for (i = 0; i < ALLOC_STEPS; ++i) {
		if (ptr >= mem_desc[i]->start && ptr <= mem_desc[i]->end) {
			bfifo_queue(mem_desc[i]->allocations, ptr);
		}
	}
}

void mem_init(void)
{
	int i, j;
	void *p;
	int fsize = sizeof(struct bfifo) + (sizeof(void *) * (ALLOC_NUM - 1));

	_heap_size = SMALLOC_SIZE;
	_heap_size += (fsize + sizeof(struct mem_desc)) * ALLOC_STEPS;

	// Calculate heap size
	for (i = 0; i < ALLOC_STEPS; ++i)
		_heap_size += (ALLOC_MIN << i) * ALLOC_NUM;

	// @@@ why does __end come out with the wrong value?
	// @@@ WARNING this restricts code+data+stack to ~1MB
	//_heap_start = __end;
	_heap_start = (void *)0x00200000;
	heap_cur = _heap_start;

	// Initialize descriptors
	for (i = 0; i < ALLOC_STEPS; ++i) {
		mem_desc[i] = smalloc(sizeof(struct mem_desc));
		if (mem_desc[i] == NULL) {
			// @@@
			return;
		}

		mem_desc[i]->least_free = ALLOC_NUM;
		mem_desc[i]->size = ALLOC_MIN << i;

		mem_desc[i]->allocations = smalloc(fsize);
		if (mem_desc[i]->allocations == NULL) {
			// @@@
			return;
		}

		memset(mem_desc[i]->allocations, 0, fsize);
		mem_desc[i]->allocations->size = ALLOC_NUM;
		mem_desc[i]->allocations->free = ALLOC_NUM;
	}

	// Initialize FIFOs
	for (i = 0; i < ALLOC_STEPS; ++i) {
		for (j = 0; j < ALLOC_NUM; ++j) {
			p = smalloc(mem_desc[i]->size);
			if (p == NULL) {
				// @@@
				return;
			}

			bfifo_queue(mem_desc[i]->allocations, p);

			if (j == 0)
				mem_desc[i]->start = p;
		}

		mem_desc[i]->end = p + mem_desc[i]->size;
	}

	// Initialization successful
	dmalloc_enabled = 1;
}

void user_page_fault(void *ptr)
{
	printf("%s: illegal pointer access at 0x%x from 0x%x, killing process\r\n",
		cur->name, ptr, cur->regs[16]);
/*
	printf("spsr: %x r14_IRQ (?): %x\r\n", cur->regs[0], cur->regs[1]);
	printf("r0: %x r1: %x r2: %x r3: %x\r\n",
		cur->regs[2], cur->regs[3], cur->regs[4], cur->regs[5]);
	printf("r4: %x r5: %x r6: %x r7: %x\r\n",
		cur->regs[6], cur->regs[7], cur->regs[8], cur->regs[9]);
	printf("r8: %x r9: %x r10: %x r11: %x\r\n",
		cur->regs[10], cur->regs[11], cur->regs[12], cur->regs[13]);
	printf("r12: %x sp: %x lr: %x\r\n",
		cur->regs[14], cur->regs[15], cur->regs[16]);
*/
	// @@@ this should be a function somewhere
	cur->state = PROC_KILLED;
	cur->timer.ticks_wakeup = 0xFFFFFFFF;
	cur->event_mask = 0;

	request_schedule();
}
