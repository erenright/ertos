/* 
 * arch/mmu.h
 *
 * MMU management
 */

#ifndef _ARCH_MMU_H
#define _ARCH_MMU_H

// Translation table base
#define TTB		0x00300000

#define MMU_SECTION		0x02	// 1MB page
#define MMU_SECTION_MASK	0xFFF00000

#define TT_BIT		0x10	// Legacy, should always be set
#define DOMAIN		0x1e0	// Domain 15

// MMU Access Permissions
#define MMU_AP_SRW_UNA	0x400	// System R/W, User No Access
#define MMU_AP_SRW_URW	0xc00	// System R/W, User R/W

void _mmu_remap(void *virt, void *phys, int flags);

#endif // !_ARCH_MMU_H
