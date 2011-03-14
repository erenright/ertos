/*
 * dev/nand.c
 *
 * Device driver for TS-7250 NAND flash.
 */

#include "../arch/regs.h"
#include <types.h>
#include "../arch/mmu.h"
#include <sleep.h>
#include <nand.h>
#include "hamming.h"

#include <stdio.h> // @@@ removeme

#define NAND_CMD_READ			0x00
#define NAND_CMD_RND_READ		0x05
#define NAND_CMD_PROGRAM_PAGE_CONFIRM	0x10
#define NAND_CMD_READ_CONFIRM		0x30
#define NAND_CMD_BLOCK_ERASE		0x60
#define NAND_CMD_READ_STATUS		0x70
#define NAND_CMD_PROGRAM_PAGE		0x80
#define NAND_CMD_RANDOM_DATA_INPUT	0x85
#define NAND_CMD_READ_ID		0x90
#define NAND_CMD_BLOCK_ERASE_CONFIRM	0xD0
#define NAND_CMD_RND_READ_CONFIRM	0xE0
#define NAND_CMD_RESET			0xFF

#define nand_busy() !(inb(NAND_BUSY) & NAND_BUSY_BIT)

#define nand_select()	outb(NAND_CTRL, inb(NAND_CTRL) | NAND_CTRL_NCE)
#define nand_deselect()	outb(NAND_CTRL, inb(NAND_CTRL) & ~NAND_CTRL_NCE)

static inline void nand_cmd(int cmd)
{
	outb(NAND_CTRL, inb(NAND_CTRL) | NAND_CTRL_CLE);
	outb(NAND_DATA, cmd);
	outb(NAND_CTRL, inb(NAND_CTRL) & ~NAND_CTRL_CLE);
}

void nand_init(void)
{
	// Remap NAND registers to give all modes access

	_mmu_remap(	(uint32_t *)NAND_CTRL,
			(uint32_t *)NAND_CTRL,
			MMU_AP_SRW_URW);

	_mmu_remap(	(uint32_t *)NAND_DATA,
			(uint32_t *)NAND_DATA,
			MMU_AP_SRW_URW);

	_mmu_remap(	(uint32_t *)NAND_BUSY,
			(uint32_t *)NAND_BUSY,
			MMU_AP_SRW_URW);
}

void nand_reset(void)
{
	// Clear all bits
	outb(NAND_CTRL, inb(NAND_CTRL) & ~(NAND_CTRL_MASK));

	nand_select();

	nand_cmd(NAND_CMD_RESET);
	while (nand_busy());

	nand_deselect();
}

uint16_t nand_read_id(void)
{
	uint16_t id;

	nand_select();

	outb(NAND_DATA, NAND_CMD_READ_ID);
	nand_cmd(NAND_CMD_READ_ID);

	outb(NAND_CTRL, inb(NAND_CTRL) | NAND_CTRL_ALE);
	outb(NAND_DATA, 0);
	outb(NAND_CTRL, inb(NAND_CTRL) & ~NAND_CTRL_ALE);

	while (nand_busy());

	id = inb(NAND_DATA) << 8;
	id |= inb(NAND_DATA);
	
	nand_deselect();

	return id;
}

// Assumes chip has been selected
static void nand_load_page(uint32_t page)
{
	int i;

	nand_cmd(NAND_CMD_READ);

	outb(NAND_CTRL, inb(NAND_CTRL) | NAND_CTRL_ALE);

	// Byte zero
	outb(NAND_DATA, 0x00);
	outb(NAND_DATA, 0x00);

	// Page
	for (i = 0; i < 3; ++i) {
		outb(NAND_DATA, page & 0xFF);
		page >>= 8;
	}

	outb(NAND_CTRL, inb(NAND_CTRL) & ~NAND_CTRL_ALE);

	nand_cmd(NAND_CMD_READ_CONFIRM);

	while(nand_busy());
}

void nand_read_page(uint32_t page, void *buf)
{
	char *p = (char *)buf;
	int i;
	uint8_t ecc[3];

	nand_select();

	nand_load_page(page);
	
	for (i = 0; i < NAND_RAW_PAGE_SIZE; ++i)
		*p++ = inb(NAND_DATA);
	
	nand_deselect();

	// Validate ECC, which is stored in the last
	// 24 bytes of the page
	for (i = 0; i < 8; ++i) {
		hamming_compute_256(buf + (i * 256), ecc);

		switch (hamming_correct_256(buf + (i * 256),
				buf + NAND_PAGE_SIZE + 40 + (i * 3),
				ecc)) {
		case 0:	// Valid ECC
			// Nop
			break;

		case HAMMING_ERROR_SINGLEBIT:
			printf("Corrected single bit error, %d\r\n", i);
			break;

		case HAMMING_ERROR_ECC:
			printf("ECC corrupted, %d\r\n", i);
			break;

		case HAMMING_ERROR_MULTIPLEBITS:
			printf("Multi-bit error, %d\r\n", i);
			break;
		}
	}
}

void nand_read(uint32_t page, int col, void *buf, size_t len)
{
	char *p = (char *)buf;
	int i;

	nand_select();

	nand_load_page(page);

	// Seek to requested byte
	nand_cmd(NAND_CMD_RND_READ);
	outb(NAND_CTRL, inb(NAND_CTRL) | NAND_CTRL_ALE);
	outb(NAND_DATA, col & 0xFF);
	outb(NAND_DATA, (col >> 8) & 0xFF);
	outb(NAND_CTRL, inb(NAND_CTRL) & ~NAND_CTRL_ALE);

	nand_cmd(NAND_CMD_RND_READ_CONFIRM);
	
	while(nand_busy());

	for (i = 0; i < len; ++i)
		*p++ = inb(NAND_DATA);
	
	nand_deselect();
}

// Expects the chip to be selected
static uint8_t nand_read_status(void)
{
	nand_cmd(NAND_CMD_READ_STATUS);

	return inb(NAND_DATA);
}

int nand_erase(uint32_t block)
{
	int status;

	// Scale erase block up to page address
	block *= NAND_PAGES_PER_BLOCK;

	nand_select();

	nand_cmd(NAND_CMD_BLOCK_ERASE);
	outb(NAND_CTRL, inb(NAND_CTRL) | NAND_CTRL_ALE);
	outb(NAND_DATA, block & 0xFF);
	outb(NAND_DATA, (block >> 8) & 0xFF);
	outb(NAND_DATA, (block >> 16) & 0xFF);
	outb(NAND_CTRL, inb(NAND_CTRL) & ~NAND_CTRL_ALE);
	nand_cmd(NAND_CMD_BLOCK_ERASE_CONFIRM);

	while (nand_busy());

	status = nand_read_status();

	nand_deselect();

	return status;
}

// Expects the chip to be selected
static void nand_random_data_input(int col)
{
	nand_cmd(NAND_CMD_RANDOM_DATA_INPUT);

	outb(NAND_CTRL, inb(NAND_CTRL) | NAND_CTRL_ALE);
	outb(NAND_DATA, col & 0xFF);
	outb(NAND_DATA, (col >> 8) & 0xFF);
	outb(NAND_CTRL, inb(NAND_CTRL) & ~NAND_CTRL_ALE);
}

int nand_write_sector(uint32_t page, int sect, void *buf)
{
	uint8_t *p = (uint8_t *)buf;
	int status, i;
	uint8_t ecc1[3];
	uint8_t ecc2[3];
	int col;

	// Calculate ECC
	hamming_compute_256(p, ecc1);
	hamming_compute_256(p + 256, ecc2);

	nand_select();

	nand_cmd(NAND_CMD_PROGRAM_PAGE);

	outb(NAND_CTRL, inb(NAND_CTRL) | NAND_CTRL_ALE);

	// Sector
	col = sect * 512;
	outb(NAND_DATA, col & 0xFF);
	outb(NAND_DATA, (col >> 8) & 0xFF);

	// Page
	for (i = 0; i < 3; ++i) {
		outb(NAND_DATA, page & 0xFF);
		page >>= 8;
	}

	outb(NAND_CTRL, inb(NAND_CTRL) & ~NAND_CTRL_ALE);

	// Stream data
	for (i = 0; i < 512; ++i)
		outb(NAND_DATA, (*p++));

	// ECC
	nand_random_data_input(NAND_PAGE_SIZE + 40 + (sect * 6));
	outb(NAND_DATA, ecc1[0]);
	outb(NAND_DATA, ecc1[1]);
	outb(NAND_DATA, ecc1[2]);
	outb(NAND_DATA, ecc2[0]);
	outb(NAND_DATA, ecc2[1]);
	outb(NAND_DATA, ecc2[2]);

	// Do it
	nand_cmd(NAND_CMD_PROGRAM_PAGE_CONFIRM);

	while(nand_busy());

	status = nand_read_status();

	nand_deselect();

	return status;
}
