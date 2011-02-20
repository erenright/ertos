include ../Makefile.inc

OBJS	= \
	uart.o

all: dev.o

dev.o: $(OBJS)
	$(LD) -r -o dev.o $(OBJS)
	cp dev.o ..

clean:
	rm -f $(OBJS) *.list dev.o
