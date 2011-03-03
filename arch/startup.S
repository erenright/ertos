#include "../config.h"

	/*
	 * Some values taken from:
	 *
	 * linux/arch/arm/include/asm/ptrace.h
	 */
	.equ	NO_IRQ,		0x80
	.equ	NO_FIQ,		0x40
	.equ	NO_INT,		(NO_IRQ | NO_FIQ)

	.equ	FIQ_MODE,	0x11
	.equ	IRQ_MODE,	0x12
	.equ	SVC_MODE,	0x13
	.equ	ABT_MODE,	0x17
	.equ	UND_MODE,	0x1B
	.equ	SYS_MODE,	0x1F

	.equ	F_BIT,		0x40
	.equ	I_BIT,		0x80

	.equ	STACK_FILL,	0xEE



	.equ	MMU_SECTION,		0x02
	.equ	TT_BIT,			0x10
	.equ	DOMAIN,			0x1e0
	.equ	MMU_AP_SRW_UNA,		0x400	/* System R/W, User no access */
	.equ	MMU_AP_SRW_URW,		0xc00	/* System R/W, User R/W */

	.text
	.code 32

	.global _start
	.func _start
_start:
	/*
	 * Vector table
	 * Used only briefly until RAM is remapped to address zero. With
	 * TS-7250/RedBoot, this has already occurred for us, but it does not
	 * hurt to have it here now. This may also be required when using JTAG.
	 */
	B	_reset		/* Reset: relative branch allows remap */
	B	.		/* Undefined instruction	*/
	B	.		/* Software interrupt		*/
	B	.		/* Prefetch abort		*/
	B	.		/* Data abort			*/
	B	.		/* Reserved			*/
	B	.		/* IRQ				*/
	B	.		/* FIQ				*/
	.endfunc

	.global _reset
	.func _reset
_reset:
	/*
	 * Call platform-specific low-level init code. Note that relative
	 * branching is required because remap has yet to occur.
	 */
	ldr	r0, =_reset
	ldr	r1, =_cstartup
	mov	lr, r1
	ldr	sp, =__stack_end__	@ Temporary stack
	b	_cstartup		@@@ should be low_level_init!
	.endfunc

	.global _cstartup
	.func _cstartup
_cstartup:
	/* Clear .bss */
	ldr	r1, =__bss_start__
	ldr	r2, =__bss_end__
	mov	r3, #0
    1:
	cmp	r1, r2
	stmltia	r1!, {r3}
	blt	1b

	/* Fill the stack with a known value */
	ldr	r1, =__stack_start__
	ldr	r2, =__stack_end__
	ldr	r3, =STACK_FILL
    1:
	cmp	r1, r2
	stmltia	r1!, {r3}
	blt	1b

	/* Initialize stack pointers for all ARM modes */
	msr	CPSR_c, #(IRQ_MODE | I_BIT | F_BIT)	@ IRQ
	ldr	sp, =__irq_stack_top__

	msr	CPSR_c, #(FIQ_MODE | I_BIT | F_BIT)	@ FIQ
	ldr	sp, =__fiq_stack_top__

	msr	CPSR_c, #(SVC_MODE | I_BIT | F_BIT)	@ SVC
	ldr	sp, =__svc_stack_top__

	msr	CPSR_c, #(ABT_MODE | I_BIT | F_BIT)	@ ABT
	ldr	sp, =__abt_stack_top__

	msr	CPSR_c, #(UND_MODE | I_BIT | F_BIT)	@ UND
	ldr	sp, =__und_stack_top__

	msr	CPSR_c, #(SYS_MODE | I_BIT | F_BIT)	@ C
	ldr	sp, =__c_stack_top__

	/*mrc	p15, 0, r1, c1, c0, 0
	orr	r1, r1, #(1 << 13)	@ map vector tables to 0xFFFF0000
	mcr	p15, 0, r1, c1, c0, 0*/

#ifdef ENABLE_MMU
	/* Enable the MMU with TTB at 0x00300000 */
	ldr	r4, =0x00300000
	mov	r1, r4
	ldr	r2, =(0x00300000+0x4000)
	mov	r3, #0
    1:
	cmp	r1, r2
	stmltia	r1!, {r3}		@ zero the TTB
	blt	1b

	mrc	p15, 0, r1, c1, c0, 0
	bic	r1, r1, #1
	mcr	p15, 0, r1, c1, c0, 0	@ ensure MMU is disabled

	mov	r1, #0
	mcr	p15, 0, r1, c7, c7, 0	@ invalidate caches
	mcr	p15, 0, r1, c8, c7, 0	@ invalidate TLBs

	mov	r1, r4
	mcr	p15, 0, r1, c2, c0, 0	@ set TTB base

	/* Map RAM 1:1 (0x00000000-0x02000000) as 1MB sections */
	mov	r3, #0			@ start with MB 0
    1:
	mov	r2, #(MMU_SECTION | TT_BIT)
	orr	r2, r2, #(DOMAIN | MMU_AP_SRW_URW)
	orr	r2, r2, r3, lsl #20
	str	r2, [r1, r3, lsl #2]	@ store at r1+(r3*4)
	add	r3, r3, #1
	cmp	r3, #32
	blt	1b

	/* Map IO 1:1 
	 * 0x80000000-0x80100000
	 * 0x80800000-0x80900000
	 */
	mov	r2, #(MMU_SECTION | TT_BIT)
	orr	r2, r2, #(DOMAIN | MMU_AP_SRW_URW)
	orr	r2, r2, #0x80000000
	ldr	r0, =(0x800 * 4)
	str	r2, [r1, r0]

	mov	r2, #(MMU_SECTION | TT_BIT)
	orr	r2, r2, #(DOMAIN | MMU_AP_SRW_URW)
	orr	r2, r2, #0x80000000
	orr	r2, r2, #0x00800000
	ldr	r0, =(0x808 * 4)
	str	r2, [r1, r0]

	/* Remap 0xFFE00000 to 0x0 (exception vectors)
	 * R/W for kernel, no access for user */
	mov	r2, #(MMU_SECTION | TT_BIT)
	orr	r2, r2, #(DOMAIN | MMU_AP_SRW_UNA)
	ldr	r0, =(0xFFE * 4)
	str	r2, [r1, r0]

	/* Remap NULL such that it is invalid */
	mov	r2, #(MMU_SECTION | TT_BIT)
	orr	r2, r2, #(DOMAIN | MMU_AP_SRW_UNA)
	str	r2, [r1]

	/* Init domains */
	@mov	r0, #(3 << 30)		@ 3 = Manager (no access checks)
	mov	r0, #(1 << 30)		@ 1 = Client (all access checks)
	mcr	p15, 0, r0, c3, c0, 0

	mrc	p15, 0, r1, c1, c0, 0
	orr	r1, r1, #4		@ enable D-cache
	@orr	r1, r1, #(1 << 12)	@ enable I-cache
	orr	r1, r1, #1		@ MMU
	mcr	p15, 0, r1, c1, c0, 0
#endif /* ENABLE_MMU */

	/* Good to go, run user code */
	bl	main

	/* Should not hit here */
	b	.

	.size	_start, . - _start
	.endfunc

	.end
