#include <types.h>

#include "../arch/regs.h"

//void __attribute__((interrupt("IRQ"))) c_irq(void)
void c_irq(void)
{
	void (*handler)(void);


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
	if (handler != NULL)
		handler();

	// Notify VIC2 that we have processed the interrupt
	outl(VIC2VectAddr, 0);
}

int register_irq_handler(int irq, void *handler)
{
	uint32_t *addr;
	uint32_t *ctrl;
	int i = 0;


	if (irq < 32) {
		addr = (uint32_t *)VIC1VectAddr0;
		ctrl = (uint32_t *)VIC1VectCntl0;
	} else if (irq < 64) {
		irq -= 32;
		addr = (uint32_t *)VIC2VectAddr0;
		ctrl = (uint32_t *)VIC2VectCntl0;
	} else {
		return -1;
	}

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

