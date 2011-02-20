include ../Makefile.inc

OBJS	= \
	processes.o \
	red.o \
	console.o

all: user.o

user.o: $(OBJS)
	$(LD) -r -o user.o $(OBJS)
	cp user.o ..

clean:
	rm -f $(OBJS) *.list user.o
