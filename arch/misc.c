/*
 * arch/misc.c
 *
 * Misc arch-specific functions.
 */

#include <types.h>
#include "regs.h"
#include <sys/irq.h>

#define WDT_CTRL (0x23800000UL)
#define WDT_FEED (0x23C00000UL)

void arch_reset(void)
{
	// On the TS-7250 we will use the CPLD watchdog for a reset
	//cli();
	//outl(WDT_FEED, 0x05);
	//outl(WDT_CTRL, 0x01);

	volatile char *feed = (volatile char *)WDT_FEED;
	volatile char *ctrl = (volatile char *)WDT_CTRL;

	*feed = 0x05;
	*ctrl = 0x01;
	while (1);
}
