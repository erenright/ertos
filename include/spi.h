/*
 * include/spi.h
 */

#ifndef _SPI_H
#define _SPI_H

void spi_loopback_test(void);
void spi_init(void);
void spi_write(const void *buf, size_t len);
void spi_read(void *buf, size_t len);

#endif // !_SPI_H
