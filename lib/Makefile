include ../Makefile.inc

OBJS	= \
	string.o \
	stdio.o \
	sleep.o \
	syscall_arm.o \
	math.o

all: lib.o

lib.o: $(OBJS)
	$(LD) -r -o lib.o $(OBJS)
	cp lib.o ..

clean:
	rm -f $(OBJS) *.list lib.o
