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
 * arch/spi.c
 *
 * EP9301 SPI device driver.
 */

#include <stdio.h>
#include <types.h>
#include "regs.h"

#define spi_busy() (inl(SSPSR) & SSPSR_BSY)
#define spi_enable() outl(SSPCR1, inl(SSPCR1) | SSPCR1_SSE)
#define spi_disable() outl(SSPCR1, inl(SSPCR1) & ~SSPCR1_SSE)

void spi_loopback_test(void)
{
	int i, c;
	int failures = 0;

	// Enable loopback mode
	spi_disable();
	outl(SSPCR1, inl(SSPCR1) | SSPCR1_LBM);
	spi_enable();

	printf("Running SPI loopback test...\r\n");

	for (i = 0; i < 16; ++i) {
		outw(SSPDR, i & 0xFF);
		while (spi_busy());

		c = inw(SSPDR);

		printf("[%d] wrote %d, read %d\r\n", i, i, c);

		if (c != i)
			++failures;
	}

	printf("Test completed with %d failures\r\n", failures);

	// Disable loopback mode
	spi_disable();
	outl(SSPCR1, inl(SSPCR1) & ~SSPCR1_LBM);
}

void spi_write(const void *buf, size_t len)
{
	const char *p = (const char *)buf;
	int i;

	spi_enable();

	for (i = 0; i < len; ++i)
		outw(SSPDR, *p++);

	spi_disable();
}
		
void spi_read(void *buf, size_t len)
{
	char *p = (char *)buf;
	int i;

	spi_enable();

	for (i = 0; i < len; ++i)
		*p++ = inw(SSPDR);

	spi_disable();
}

void spi_init(void)
{
	// Set DIO_8 (PortF:1) and PortC:0 to HIGH, which is !CS on the HMM
	outl(PFDDR, inl(PFDDR) | 0x02);
	outl(PFDR, inl(PFDR) | 0x02);
	outl(PCDDR, inl(PCDDR) | 0x01);
	outl(PCDR, inl(PCDR) | 0x01);

	// Initialize SPI to interface with AT45DB321B
	spi_enable();
	outl(SSPCPSR, 2);	// Lowest possible divisor

	outl(SSPCR0, (SSPCR0_DSS_8_BIT << SSPCR0_DSS_SHIFT));	// 8-bit
								// SPI mode 0
								// No SCR
	spi_disable();
	spi_enable();
}
