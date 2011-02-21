/*
 * user/red.c
 *
 * Simple user-mode process to toggle the red LED.
 */

#include "../arch/regs.h"

#include <stdio.h>
#include <sleep.h>

static volatile uint8_t *leds = (uint8_t *)LEDS;

static void handle_alarm(void)
{
	int i;

	for (i = 0; i < 10; ++i) {
		*leds ^= LED_RED;
		sleep(100);
	}
}

void red_task(void)
{
	struct alarm a;

	a.oneshot = 0;
	a.msec = 10000;
	a.handler = handle_alarm;
	alarm(&a);

	*leds &= ~LED_RED;

	while (1) {
		// Sleep 1 second
		sleep(1000);

		*leds ^= LED_RED;
	}
}

