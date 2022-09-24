/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2022, Eric Enright
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
