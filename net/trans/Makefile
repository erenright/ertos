include ../../Makefile.inc

OBJS	= \
	icmp.o \
	ip.o

all: trans.o

trans.o: $(OBJS)
	$(LD) -r -o trans.o $(OBJS)
	cp trans.o ..

clean:
	rm -f $(OBJS) *.list trans.o
