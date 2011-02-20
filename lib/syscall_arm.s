	.text
	.code 32
	.align 4

	.global __syscall
	.func __syscall
__syscall:
	stmdb	sp!, {r0-r3,r12,lr}

	/* no svc on compiler? same as swi */
	swi	0x0			@ real number is in r0

	ldmia	sp!, {r0-r3,r12,pc}

	.endfunc

	.end
