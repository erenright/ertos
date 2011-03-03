include ../Makefile.inc

OBJS	= \
	core.o \
	dll.o \
	trans.o

all: net.o

net.o: $(OBJS)
	$(LD) -r -o net.o $(OBJS)
	cp net.o ..

core.o: force_look
	$(MAKE) -C core

dll.o: force_look
	$(MAKE) -C dll

trans.o: force_look
	$(MAKE) -C trans

force_look:
	true

clean:
	rm -f *.o
	for dir in $(OBJS); do \
		$(MAKE) -C `basename $$dir .o` clean; \
	done

