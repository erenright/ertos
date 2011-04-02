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
