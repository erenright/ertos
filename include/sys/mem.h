#ifndef _MEM_H
#define _MEM_H

#include <types.h>

extern void *_heap_start;
extern size_t _heap_size;

void mem_init(void);
void * malloc(size_t size);
void free(void *ptr);

#endif // !_MEM_H
