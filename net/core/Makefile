include ../../Makefile.inc

OBJS	= \
	pkt.o

all: core.o

core.o: $(OBJS)
	$(LD) -r -o core.o $(OBJS)
	cp core.o ..

clean:
	rm -f $(OBJS) *.list core.o
