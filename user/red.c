/*
 * user/red.c
 *
 * Simple user-mode process to toggle the red LED.
 */

#include "../arch/regs.h"

#include <stdio.h>
#include <sleep.h>

void red_task(void)
{
	volatile uint8_t *leds = (uint8_t *)LEDS;

	*leds &= ~LED_RED;

	while (1) {
		// Sleep 1 second
		sleep(100);

		*leds ^= LED_RED;
	}
}

