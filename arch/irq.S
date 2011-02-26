	/*
	 * @@@ also used in arch/cpu.s, must share these
	 */
	.equ	NO_IRQ,		0x80
	.equ	NO_FIQ,		0x40
	.equ	NO_INT,		(NO_IRQ | NO_FIQ)

	.equ	USR_MODE,	0x10
	.equ	FIQ_MODE,	0x11
	.equ	IRQ_MODE,	0x12
	.equ	SVC_MODE,	0x13
	.equ	ABT_MODE,	0x17
	.equ	UND_MODE,	0x1B
	.equ	SYS_MODE,	0x1F
	.equ	MODE_MASK,	0x1F

	.text
	.code 32


	.global read_RescheduleFlag
	.func read_RescheduleFlag
read_RescheduleFlag:
	ldr	r0, =_need_reschedule
	ldr	r0, [r0]
	mov	pc, lr
	.endfunc


	.data
	.align 4
handler_irqstack:
	.word	0x0

	.text
	.align 4


	.global arm_svc_entry
	.func arm_svc_entry
arm_svc_entry:
	stmdb	sp!, {r0-r3,r12,lr}

	mov	r1, sp			@ arg 2 of c_svc is ptr to regs
	mrs	r0, spsr
	stmdb	sp!, {r0, r3}		@ store spsr and another reg to
					@ maintain 8-byte-aligned stack

	ldr	r0, [lr, #-4]		@ load calling instruction
	bic	r0, r0, #0xFF000000	@ extract comment field

	/* call c_svc(syscall_number[r0], reg ptr[r1]) */
	bl	c_svc

	bl	read_RescheduleFlag	@ does the scheduler want to run?
					@ @@@ from arch/irq.s
	cmp	r0, #1
	bleq	cpu_idle		@ yes, idle this task

	ldmia	sp!, {r0, r3}		@ restore spsr
	msr	spsr_cxsf, r0
	ldmia	sp!, {r0-r3,r12,pc}^
	nop
	.endfunc


	.global arm_irq_entry
	.func arm_irq_entry
arm_irq_entry:
	sub	lr, lr, #4		@ correct return address
	stmdb	sp!, {r0-r3,r12,lr}	@ store regs in sp_irq

	bl	c_irq			@ service the interrupt

	bl	read_RescheduleFlag	@ rescheduling required?
	cmp	r0, #0
	ldmeqia	sp!, {r0-r3,r12,pc}^	@ no, so return

	ldmia	sp!, {r0-r3,r12,lr}	@ restore original regs

	/*
	 * Scheduling and context-switch code follows
	 */

	/* Record original sp_IRQ in handler_irqstack  */
	stmdb	sp!, {r0,r1}		@ free r0 and r1 so we can save sp
	mov	r0, sp
	add	r0, r0, #8		@ compensate 8 bytes from r0,r1 store
	ldr	r1, =handler_irqstack
	str	r0, [r1]		@ remember original stack for later
	ldmia	sp!, {r0, r1}		@ restore r0 and r1

	/* Store current task */
	ldr	sp, =cur		@ load ptr to current task
	ldr	sp, [sp]
	add	sp, sp, #24		@ +(6*4) for proc->regs[2]

	stmia	sp, {r0-r14}^		@ store user regs
	sub	sp, sp, #8		@ -(2*4) for proc->regs[0]
	mrs	r0, spsr
	stmia	sp, {r0,r14}		@ store spsr and lr

	/* Restore sp_IRQ for the following C call */
	ldr	sp, =handler_irqstack
	ldr	sp, [sp]

	/* Call the scheduler */
	bl	schedule		@ call the scheduler
	ldr	r0, =cur
	ldr	r0, [r0]

	/* Load next task */
	ldr	sp, =cur		@ load ptr to next task
	ldr	sp, [sp]
	add	sp, sp, #16		@ +(4*4) for proc->regs
	ldmia	sp!, {r0,lr}		@ load user spsr and r14_irq

	/* --- */
	@ldr	r0, =0x80840020
	@ldrb	r1, [r0, #0]
	@orr	r1, r1, #2
	@strb	r1, [r0, #0]
	/* --- */

	msr	spsr_cxsf, r0		@ copy over the spsr
	ldmia	sp, {r0-r14}^		@ load the rest of the user regs
	nop				@ should follow ldmia w/ ^

	/* Restore original sp_IRQ from handler_irqstack */
	ldr	sp, =handler_irqstack
	ldr	sp, [sp]		@ reload sp_IRQ

	movs	pc, lr			@ return

	.endfunc


	.global sti
	.func sti
sti:
	stmfd	sp!, {r0,lr}		@ save regs
	mrs	r0, cpsr		@ load the existing CPSR
	bic	r0, r0, #NO_IRQ		@ enable IRQ
	msr	cpsr_c, r0		@ set new CPSR
	ldmfd	sp!, {r0,pc}		@ restore regs and return
	.endfunc


	.global cli
	.func cli
cli:
	stmfd	sp!, {r0,lr}		@ save regs
	mrs	r0, cpsr		@ load the existing CPSR
	orr	r0, r0, #NO_IRQ		@ disable IRQ
	msr	cpsr_c, r0		@ set new CPSR
	ldmfd	sp!, {r0,pc}		@ restore regs and return
	.endfunc


	.global enable_irq
	.func enable_irq
enable_irq:
	stmdb	sp!, {r4-r7,lr}

	mov	r4, r0			@ make a copy
	mov	r5, #0x80000010		@ Base register

	cmp	r4, #32			@ IRQ less than 32?
	blt	.ENvicone		@ Yes, use VIC1

	cmp	r4, #63			@ IRQ >= 64?
	bgt	.ENdone			@ Yes, don't deal with invalid IRQ

	sub	r4, r4, #32		@ Scale for VIC2
	add	r5, r5, #0x000C0000	@ Bump fully to VIC2IntEnable
	b	.ENdoit

.ENvicone:
	add	r5, r5, #0x000B0000	@ Bump fully to VIC1IntEnable

.ENdoit:
	mov	r6, #1
	mov	r4, r6, lsl r4		@ Get IRQ bit (1 << r4)

	@@@ disable other interrupts here??
	ldr	r7, [r5]		@ pull in existing reg
	orr	r7, r7, r4		@ bring in the new IRQ
	str	r7, [r5]		@ Enable the IRQ

.ENdone:
	ldmfd	sp!, {r4-r7,pc}
	.endfunc


	.global disable_irq
	.func disable_irq
disable_irq:
	stmdb	sp!, {r4-r6,lr}

	mov	r4, r0			@ make a copy
	mov	r5, #0x80000014		@ Base register

	cmp	r4, #32			@ IRQ less than 32?
	blt	.DISvicone		@ Yes, use VIC1

	cmp	r4, #63			@ IRQ >= 64?
	bgt	.DISdone		@ Yes, don't deal with invalid IRQ

	sub	r4, r4, #32		@ Scale for VIC2
	add	r5, r5, #0x000C0000	@ Bump fully to VIC2IntEnClear
	b	.DISdoit

.DISvicone:
	add	r5, r5, #0x000B0000	@ Bump fully to VIC1IntEnClear

.DISdoit:
	mov	r6, #1
	mov	r4, r6, lsl r4		@ Get IRQ bit (1 << r4)
	str	r4, [r5]		@ Disable the IRQ

.DISdone:
	ldmfd	sp!, {r4-r6,pc}
	.endfunc


	.end

