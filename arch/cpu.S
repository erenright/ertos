	/* @@@ stolen from arch/irq.s must share these */
	.equ	IRQ_MASK,	0x80
	.equ	FIQ_MASK,	0x40
	.equ	INT_MASK,	(IRQ_MASK | FIQ_MASK)

	.equ	USR_MODE,	0x10
	.equ	FIQ_MODE,	0x11
	.equ	IRQ_MODE,	0x12
	.equ	SVC_MODE,	0x13
	.equ	ABT_MODE,	0x17
	.equ	UND_MODE,	0x1B
	.equ	SYS_MODE,	0x1F
	.equ	MODE_MASK,	0x1F

	.equ	FLAG_MASK,	0xF0000000
	.equ	V_FLAG,		0x10000000
	.equ	C_FLAG,		0x20000000
	.equ	Z_FLAG,		0x40000000
	.equ	N_FLAG,		0x80000000


	.section .rodata
	.align 2
crlf_str:
	.string "\r\n"
und_str:
	.string "\r\nTRAP: Undefined Instruction\r\n"
pa_str:
	.string "\r\nTRAP: Prefetch Abort\r\n"
reset_str:
	.string "\r\nTRAP: Reset\r\n"
da_str:
	.string "\r\nTRAP: Data Abort\r\n"
cpsr_str:
	.string "CPSR: "
spsr_str:
	.string "SPSR: "
irq_str:
	.string "IRQ "
fiq_str:
	.string "FIQ "
on_str:
	.string "on "
off_str:
	.string "off "
usrm_str:
	.string "USR_mode "
fiqm_str:
	.string "FIQ_mode "
irqm_str:
	.string "IRQ_mode "
svcm_str:
	.string "SVC_mode "
abtm_str:
	.string "ABT_mode "
undm_str:
	.string "UND_mode "
sysm_str:
	.string "SYS_mode "
lr_str:
	.string "lr: "
vec_str:
	.string "Vector tables:\r\n"
kernel_reg_str:
	.string "Kernel registers:\r\n"
user_reg_str:
	.string "User registers:\r\n"

	.text
	.code 32
	.align 4


	.func print_psr
	/* Entry: r0 = psr */
print_psr:
	stmdb	sp!, {r4-r5,lr}

	mov	r5, r0

	bl	_printf_x		@ print psr
	mov	r0, #' '
	bl	_putchar
	mov	r0, #'('
	bl	_putchar

	and	r4, r5, #FLAG_MASK	@ inform the user what flags are set
	and	r0, r4, #N_FLAG
	cmp	r0, #0
	movne	r0, #'N'
	blne	_putchar
	and	r0, r4, #Z_FLAG
	cmp	r0, #0
	movne	r0, #'Z'
	blne	_putchar
	and	r0, r4, #C_FLAG
	cmp	r0, #0
	movne	r0, #'C'
	blne	_putchar
	and	r0, r4, #V_FLAG
	cmp	r0, #0
	movne	r0, #'V'
	blne	_putchar

	mov	r0, #' '
	bl	_putchar

	and	r4, r5, #MODE_MASK	@ inform the user what mode it is
	cmp	r4, #USR_MODE
	ldreq	r0, =usrm_str
	bleq	_puts
	cmp	r4, #FIQ_MODE
	ldreq	r0, =fiqm_str
	bleq	_puts
	cmp	r4, #IRQ_MODE
	ldreq	r0, =irqm_str
	bleq	_puts
	cmp	r4, #SVC_MODE
	ldreq	r0, =svcm_str
	bleq	_puts
	cmp	r4, #ABT_MODE
	ldreq	r0, =abtm_str
	bleq	_puts
	cmp	r4, #UND_MODE
	ldreq	r0, =undm_str
	bleq	_puts
	cmp	r4, #SYS_MODE
	ldreq	r0, =sysm_str
	bleq	_puts

	and	r4, r5, #INT_MASK	@ inform the user what interrupts are on

	ldr	r0, =irq_str
	bl	_puts

	and	r0, r4, #IRQ_MASK
	cmp	r0, #0
	ldrne	r0, =off_str
	ldreq	r0, =on_str
	bl	_puts

	mov	r0, #' '
	bl	_putchar

	ldr	r0, =fiq_str
	bl	_puts

	and	r0, r4, #FIQ_MASK
	cmp	r0, #0
	ldrne	r0, =off_str
	ldreq	r0, =on_str
	bl	_puts

	mov	r0, #')'
	bl	_putchar

	ldr	r0, =crlf_str
	bl	_puts

	ldmia	sp!, {r4-r5,pc}
	.endfunc


	.func print_regs
	/* Expects {r0-r12,lr} on sp */
print_regs:
	mov	r7, lr

	mov	r4, #0			@ reg counter 1
	mov	r5, #0xFFFFFFFF		@ reg counter 2
	mov	r6, #0			@ row counter
.Lnextreg:
	mov	r0, #'r'		@ print "r?: "
	bl	_putchar
	cmp	r4, #10
	addlt	r0, r4, #'0'
	movge	r0, #'1'
	bl	_putchar
	cmp	r5, #0xFFFFFFFF
	addne	r0, r5, #'0'
	blne	_putchar
	mov	r0, #':'
	bl	_putchar
	mov	r0, #' '
	bl	_putchar

	ldmia	sp!, {r0}		@ load reg
	bl	_printf_x
	mov	r0, #' '
	bl	_putchar

	add	r6, r6, #1
	cmp	r6, #4			@ must we print CRLF?
	bne	.Lnocrlf
	mov	r6, #0
	ldr	r0, =crlf_str		@ 4 regs printed, do CRLF
	bl	_puts

.Lnocrlf:
	add	r4, r4, #1
	cmp	r4, #10
	addge	r5, r5, #1		@ bump r5 if r4 >= 10
	cmp	r4, #13
	bne	.Lnextreg

	ldr	r0, =lr_str		@ lr is special as we want "lr: "
	bl	_puts
	ldmia	sp!, {r0}
	bl	_printf_x
	ldr	r0, =crlf_str
	bl	_puts

	mov	pc, r7
	.endfunc


	.func print_vec
print_vec:
	mov	r7, lr
	mov	r5, #0			@ start with vector table 1
	mov	r6, #0

.Lagain:
	ldr	r0, [r5, #0]
	bl	_printf_x		@ reset
	mov	r0, #' '
	bl	_putchar
	ldr	r0, [r5, #4]
	bl	_printf_x		@ undefined instruction
	mov	r0, #' '
	bl	_putchar
	ldr	r0, [r5, #8]
	bl	_printf_x		@ software interrupt
	mov	r0, #' '
	bl	_putchar
	ldr	r0, [r5, #12]
	bl	_printf_x		@ prefetch abort
	mov	r0, #' '
	bl	_putchar
	mov	r0, #' '
	bl	_putchar
	ldr	r0, [r5, #16]
	bl	_printf_x		@ data abort
	mov	r0, #' '
	bl	_putchar
	ldr	r0, [r5, #20]
	bl	_printf_x		@ reserved
	mov	r0, #' '
	bl	_putchar
	ldr	r0, [r5, #24]
	bl	_printf_x		@ irq
	mov	r0, #' '
	bl	_putchar
	ldr	r0, [r5, #28]
	bl	_printf_x		@ fiq
	ldr	r0, =crlf_str
	bl	_puts

	add	r5, r5, #32		@ offset to vector table 2

	cmp	r6, #0
	addeq	r6, r6, #1
	beq	.Lagain

	mov	pc, r7

	.endfunc


	.func generic_exception_handler
generic_exception_handler:

	ldr	r0, =crlf_str
	bl	_puts

	ldr	r0, =cpsr_str
	bl	_puts
	mrs	r0, CPSR
	bl	print_psr

	ldr	r0, =spsr_str
	bl	_puts
	mrs	r0, SPSR
	bl	print_psr

	ldr	r0, =crlf_str
	bl	_puts

	ldr	r0, =kernel_reg_str
	bl	_puts
	bl	print_regs		@ stack is unloaded after this

	ldr	r0, =crlf_str
	bl	_puts

	ldr	r0, =user_reg_str
	bl	_puts
	stmdb	sp!, {r0-r12,lr}^	@ pull in user registers
	bl	print_regs		@ stack is unloaded after this

	ldr	r0, =crlf_str
	bl	_puts

	ldr	r0, =vec_str
	bl	_puts
	bl	print_vec

	b	.			@ nothing more to do
	.endfunc


	.global arm_reset_entry
	.func arm_reset_entry
arm_reset_entry:
	stmdb	sp!, {r0-r12,lr}

	ldr	r0, =reset_str
	bl	_puts

	b	generic_exception_handler	@ never returns
	.endfunc


	.global arm_pa_entry
	.func arm_pa_entry
arm_pa_entry:
	stmdb	sp!, {r0-r12,lr}

	ldr	r0, =pa_str
	bl	_puts

	b	generic_exception_handler	@ never returns
	.endfunc


	.global arm_und_entry
	.func arm_und_entry
arm_und_entry:
	stmdb	sp!, {r0-r12,lr}

	ldr	r0, =und_str
	bl	_puts

	b	generic_exception_handler	@ never returns
	.endfunc


	.global arm_da_entry
	.func arm_da_entry
arm_da_entry:
	stmdb	sp!, {r0-r12,lr}
	mrs	r0, spsr
	and	r0, r0, #MODE_MASK
	cmp	r0, #USR_MODE			@ was this a user-mode error?
	bne	.Ldogeneric			@ no

	@ Yes, handle it
	mov	r0, sp				@ remember current sp
	ldr	sp, =cur			@ load ptr to current task
	ldr	sp, [sp]
	cmp	sp, #0
	beq	.Ldogeneric			@ fatal: no current task!
	add	sp, sp, #16			@ +(4*4) for proc->regs

	mrs	r1, spsr
	stmia	sp!, {r1,r14}			@ store spsr and lr
	stmia	sp!, {r0-r14}			@ store user regs

	mov	sp, r0				@ restore irq stack
	bl	_user_page_fault		@ handle page fault

	@@@@ enable irq
	mrs	r0, cpsr
	bic	r0, r0, #IRQ_MASK
	msr	cpsr_cxfs, r0
	b	.

	bl	cpu_idle			@ wait
	ldmia	sp!, {r0-r12,pc}		@ return from whence we came
						@ @@@ this may break if the
						@ C handler kills the proc!
.Ldogeneric:
	ldr	r0, =da_str
	bl	_puts

	b	generic_exception_handler	@ never returns
	.endfunc


	.global cpu_idle
	.func cpu_idle
cpu_idle:
	mcr	p15, 0, r0, c7, c0, 4	@ wait for interrupt
	mov	pc, lr
	.endfunc



	.end
