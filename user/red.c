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

