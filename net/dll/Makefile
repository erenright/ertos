include ../../Makefile.inc

OBJS	= \
	arp.o \
	eth.o

all: dll.o

dll.o: $(OBJS)
	$(LD) -r -o dll.o $(OBJS)
	cp dll.o ..

clean:
	rm -f $(OBJS) *.list dll.o
