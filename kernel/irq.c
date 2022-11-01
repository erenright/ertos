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
	if (handler == arm_irq_entry)
		++kstat.isr_recursion;
	else if (handler != NULL)
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
		sel  = (uint32_t *)VIC2IntSelect;
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

