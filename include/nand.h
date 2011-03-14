/*
 * include/nand.h
 *
 * NAND device interface.
 */

#ifndef _NAND_H
#define _NAND_H

#include <types.h>

#define NAND_RAW_PAGE_SIZE	2112
#define NAND_PAGE_SIZE		2048
#define NAND_NUM_PAGES		((128*1024*1024) / NAND_PAGE_SIZE)

#define NAND_PAGES_PER_BLOCK	64

void nand_init(void);
void nand_reset(void);
uint16_t nand_read_id(void);
void nand_read_page(uint32_t page, void *buf);
void nand_read(uint32_t page, int col, void *buf, size_t len);
int nand_erase(uint32_t block);
int nand_write_sector(uint32_t page, int sect, void *buf);

#endif // !_NAND_H
