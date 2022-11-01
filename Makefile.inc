CROSS	= arm-none-eabi-
CC	= $(CROSS)gcc
AS	= $(CROSS)as
LD	= $(CROSS)ld
OBJCOPY	= $(CROSS)objcopy

CFLAGS	= -Wall -Werror -fpic -ffreestanding -ggdb -I/home/eric/src/ertos/include
#CFLAGS	= -Wall -fpic -ffreestanding -ggdb -I../include

.c.o:
	$(CC) $(CFLAGS) -Wa,-ahdl=$<.list -c -o $@ $<

.S.o:
	$(CC) $(ASFLAGS) -c -o $@ $<
.s.o:
	$(CC) $(ASFLAGS) -c -o $@ $<
