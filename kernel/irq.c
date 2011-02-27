#include <types.h>
#include <sys/sched.h>
#include <sys/kernel.h>
#include <kstat.h>

#include "../arch/regs.h"

void arm_irq_entry(void);

void c_irq(void)
{
	void (*handler)(void);

	self = kernel_self;

/*
	// Notify VIC1 that we are processing the interrupt
	handler = (void *)inl(VIC1VectAddr);

	// Service the interrupt
	if (handler != NULL)
		handler();

	// Notify VIC1 that we have processed the interrupt
	outl(VIC1VectAddr, 0);
*/

	// Notify VIC2 that we are processing the interrupt
	handler = (void *)inl(VIC2VectAddr);

	// Service the interrupt
	// Avoid recursion into arm_irq_entry, @@@ why is it showing up??
	if (handler == arm_irq_entry)
		++kstat.isr_recursion;
	else if (handler != NULL)
		handler();

	// Notify VIC2 that we have processed the interrupt
	outl(VIC2VectAddr, 0);

	self = cur->self;
}

int register_irq_handler(int irq, void *handler, int fast)
{
	uint32_t *addr;
	uint32_t *ctrl;
	uint32_t *sel;
	int i = 0;


	if (irq < 32) {
		addr = (uint32_t *)VIC1VectAddr0;
		ctrl = (uint32_t *)VIC1VectCntl0;
		sel  = (uint32_t *)VIC1IntSelect;
	} else if (irq < 64) {
		irq -= 32;
		addr = (uint32_t *)VIC2VectAddr0;
		ctrl = (uint32_t *)VIC2VectCntl0;
		sel  = (uint32_t *)VIC1IntSelect;
	} else {
		return -1;
	}

	i = inl(sel);
	if (fast)
		i |= (1 << irq);
	else
		i &= ~(1 << irq);
	outl(sel, i);

	// Look for an open vector
	for (i = 0; i < 16; ++i) {
		if ((uint32_t *)inl(addr + (i * 4)) == NULL) {
			// Found one
			outl(addr + (i * 4), handler);
			outl(ctrl + (i * 4), irq | INT_ENABLE);
			return 0;
		}
	}

	// Failed to find an open vector
	return -1;
}

