ENTRY(_start)

C_STACK_SIZE	= 4096;
IRQ_STACK_SIZE	= 4096;
FIQ_STACK_SIZE	= 4096;
SVC_STACK_SIZE	= 4096;
ABT_STACK_SIZE	= 4096;
UND_STACK_SIZE	= 4096;

HEAP_SIZE	= 0x100000;	/* 1MB heap */
RAM_START	= 0x00100000;

SECTIONS
{
	. = RAM_START;
	.startup . : { startup.o }

	.text : { *(.text) }

	.data : {
		KEEP(*(.jcr))
		*(.got.plt) *(.got)
		*(.shdata)
		*(.data .data.* .gnu.linkonce.d.*)
	}

	.bss : {
		__bss_start__ = .;
		*(.bss)
		__bss_end__ = .;
	}

	.rodata : { *(.rodata) }

	.stack : {
		. = ALIGN(4);
		__stack_start__ = .;

		. += IRQ_STACK_SIZE;
		. = ALIGN(4);
		__irq_stack_top__ = .;

		. += FIQ_STACK_SIZE;
		. = ALIGN(4);
		__fiq_stack_top__ = .;

		. += SVC_STACK_SIZE;
		. = ALIGN(4);
		__svc_stack_top__ = .;

		. += ABT_STACK_SIZE;
		. = ALIGN(4);
		__abt_stack_top__ = .;

		. += UND_STACK_SIZE;
		. = ALIGN(4);
		__und_stack_top__ = .;

		. += C_STACK_SIZE;
		. = ALIGN(4);
		__c_stack_top__ = .;

		__stack_end__ = .;
	}

	__end__ = .;
}
