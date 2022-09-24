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
 * this list of conditions and the following disclaimer.
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
 * arch/mmu.c
 *
 * EP93xx memory management.
 */

#include <types.h>
#include <stdio.h>

#include "mmu.h"

// From "ARM920T Technical Reference Manual" section 3.5.1
#define FSR_FAULT_MASK			0x0F
#define FSR_FAULT_ALIGNMENT		0x01
#define FSR_FAULT_TRANSLATION_SECTION	0x05
#define FSR_FAULT_TRANSLATION_PAGE	0x07
#define FSR_FAULT_DOMAIN_SECTION	0x09
#define FSR_FAULT_DOMAIN_PAGE		0x0B
#define FSR_FAULT_PERMISSION_SECTION	0x0D
#define FSR_FAULT_PERMISSION_PAGE	0x0F
#define FSR_FAULT_EXTERNAL_ABT_SECTION	0x08
#define FSR_FAULT_EXTERNAL_ABT_PAGE	0x0A

static const char *fsr_faults[] = {
	"Unknown",			// 0x00
	"Alignment",			// 0x01
	"Unknown",			// 0x02
	"Unknown",			// 0x03
	"Unknown",			// 0x04
	"Translation (section)",	// 0x05
	"Unknown",			// 0x06
	"Translation (page)",		// 0x07
	"External Abort (section)",	// 0x08
	"Domain (section)",		// 0x09
	"External Abort (page)",	// 0x0A
	"Domain (page)",		// 0x0B
	"Unknown",			// 0x0C
	"Permission (section)",		// 0x0D
	"Permission (page)",		// 0x0F
};

// Pointer to MMU translation table
static uint32_t *ttb = (uint32_t *)TTB;

// @@@ kernel/mem.c
void user_page_fault(void *ptr);

// Called from arm_da_entry when a Data Abort exception was generated
// from USR_mode
void _user_page_fault(void)
{
	uint32_t fsr;
	void *far;

	asm(	"mrc	p15, 0, %[fsr], c5, c0, 0	\r\n"
		"mrc	p15, 0, %[far], c6, c0, 0	\r\n"
		: [fsr] "=r" (fsr),
		  [far] "=r" (far)
	);

	fsr &= FSR_FAULT_MASK;

	printf("_user_page_fault: %s\r\n", fsr_faults[fsr]);

	user_page_fault(far);
}

// Called from arm_da_entry when a Data Abort exception was generated
// from SYS_mode
void _system_page_fault(void)
{
	uint32_t fsr;
	void *far;

	asm(	"mrc	p15, 0, %[fsr], c5, c0, 0	\r\n"
		"mrc	p15, 0, %[far], c6, c0, 0	\r\n"
		: [fsr] "=r" (fsr),
		  [far] "=r" (far)
	);

	fsr &= FSR_FAULT_MASK;

	printf("_system_page_fault: %s\r\n", fsr_faults[fsr]);

	user_page_fault(far);
}

#define mmu_invalidate_caches() \
	asm(	"mov	r1, #0			\r\n"	\
		"mcr	p15, 0, r1, c7, c7, 0	\r\n"	\
		::: "r1"				\
	)

#define mmu_invalidate_tlb() \
	asm(	"mov	r1, #0			\r\n"	\
		"mcr	p15, 0, r1, c8, c7, 0	\r\n"	\
		::: "r1"				\
	)

void _mmu_remap(void *virt, void *phys, int flags)
{
	uint32_t entry = 0;
	int idx = ((uint32_t)virt & MMU_SECTION_MASK) >> 20;

	flags |= TT_BIT;	// Legacy ARM cruft
	flags |= DOMAIN;
	flags |= MMU_SECTION;

	entry = (uint32_t)phys & MMU_SECTION_MASK;
	entry |= flags & (~MMU_SECTION_MASK);

	ttb[idx] = entry;

	mmu_invalidate_caches();
	mmu_invalidate_tlb();
}
