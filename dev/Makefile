include ../Makefile.inc

OBJS	= \
	uart.o \
	nand.o \
	hamming.o \
	ohci.o \
	usb.o

all: dev.o

dev.o: $(OBJS)
	$(LD) -r -o dev.o $(OBJS)
	cp dev.o ..

clean:
	rm -f $(OBJS) *.list dev.o
