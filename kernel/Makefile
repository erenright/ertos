include ../Makefile.inc

OBJS	= \
	main.o \
	irq.o \
	cons.o \
	sched.o \
	mem.o \
	sem.o \
	syscall.o \
	timers.o \
	list.o

all: kernel.o

kernel.o: $(OBJS)
	$(LD) -r -o kernel.o $(OBJS)
	cp kernel.o ..

clean:
	rm -f $(OBJS) *.list kernel.o
