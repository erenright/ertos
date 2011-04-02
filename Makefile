include Makefile.inc

#INSTALLDIR = ~/public_html
INSTALLDIR = /var/www/html

OBJS = \
	arch.o \
	kernel.o \
	lib.o \
	dev.o \
	net.o \
	user.o

all: ertos

ertos: $(OBJS)
	$(LD) -Map=ertos.map -T ertos.ld -o ertos.elf $(OBJS)
	$(OBJCOPY) -O binary ertos.elf ertos.bin
	ls -lh ertos.elf ertos.bin

arch.o: force_look
	$(MAKE) -C arch

kernel.o: force_look
	$(MAKE) -C kernel

lib.o: force_look
	$(MAKE) -C lib

dev.o: force_look
	$(MAKE) -C dev

net.o: force_look
	$(MAKE) -C net

user.o: force_look
	$(MAKE) -C user

force_look:
	true

install:
	cp ertos.elf $(INSTALLDIR)
	cp ertos.bin $(INSTALLDIR)

clean:
	rm -f *.o ertos.elf ertos.bin ertos.map
	for dir in $(OBJS); do \
		$(MAKE) -C `basename $$dir .o` clean; \
	done

