include ../Makefile.inc
include ../components

ETH_O-$(CONFIG_NET) += eth.o
SPI_O-$(CONFIG_SPI) += spi.o

OBJS	= \
	init.o \
	irq.o \
	uart.o \
	sem.o \
	cpu.o \
	mmu.o \
	misc.o \
	$(ETH_O-y) \
	$(SPI_O-y)

all: arch.o

arch.o: $(OBJS) startup.o
	$(LD) -r -o arch.o $(OBJS)
	cp arch.o ..
	cp startup.o ..

clean:
	rm -f $(OBJS) *.list startup.o arch.o
