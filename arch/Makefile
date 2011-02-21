include ../Makefile.inc

OBJS	= \
	init.o \
	irq.o \
	uart.o \
	sem.o \
	cpu.o \
	mmu.o \
	misc.o

all: arch.o

arch.o: $(OBJS) startup.o
	$(LD) -r -o arch.o $(OBJS)
	cp arch.o ..
	cp startup.o ..

clean:
	rm -f $(OBJS) *.list startup.o arch.o
