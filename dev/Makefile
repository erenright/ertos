include ../Makefile.inc
include ../components

NAND_O-$(CONFIG_NAND) += nand.o hamming.o
USB_O-$(CONFIG_USB) += ohci.o usb.o

OBJS	= \
	uart.o \
	$(NAND_O-y) \
	$(USB_O-y)

all: dev.o

dev.o: $(OBJS)
	$(LD) -r -o dev.o $(OBJS)
	cp dev.o ..

clean:
	rm -f $(OBJS) *.list dev.o
