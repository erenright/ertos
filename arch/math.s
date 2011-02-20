	.text
	.code 32

	.global __divsi3
	.func __divsi3
__divsi3:
	stmdb	sp!, {r4-r5,lr}

	cmp	r1, #0
	beq	divide_by_zero		@ avoid divide by zero

	mov	r4, #0			@ clear r0 for result accumulation
	mov	r5, #1			@ initial bit shift

divide_start:
	cmp	r1, r0
	movls	r1, r1, lsl #1
	movls	r5, r5, lsl #1
	bls	divide_start

divide_next:
	cmp	r0, r1			@ carry set if r0 > r1
	subcs	r0, r0, r1		@ substract r1 from r0 if positive
	addcs	r4, r4, r5		@ add the current bit in r5 to
					@ accumulating answer in r4

	movs	r5, r5, lsr #1		@ shift r5 into carry flag
	movcc	r1, r1, lsr #1		@ if bit 0 of r3 was 0, shift r1 right
	bcc	divide_next		@ if NC, r3 shifted back to where it
					@ started, so we can end

	mov	r0, r4			@ move result into r0
	b	divide_end

divide_by_zero:				@@@ trap
	mov	r0, #0			@ return zero on divide by zero

divide_end:
	ldmfd	sp!, {r4-r5,pc}
	.endfunc


	.global __modsi3
	.func __modsi3
__modsi3:
	cmp	r1 #0
	beq	mod_by_zero

	rsbmi	r1, r1, #0		@ loops below use unsigned
	movs	ip, r0			@ preserve sign of dividend
	rsbmi	r0, r0, #0		@ if negative, make positive
	subs	r2, r1, #1		@ compare divisor with 1
	cmpne	r0, r1			@ compare dividend with divisor
	moveq	r0, #0
	tsthi	r1, r2			@ see if divisor is power of 2
	

mod_by_zero:
	mov	r0, #0			@@@ trap
	mov	pc, lr
	.endfunc
