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
	.align 4

	.global sem_try_down
	.func sem_try_down
	/*
	 * Entry: r0: (sem_t *)
	 *
	 * Return: r0 is 0 if locked, non-zero otherwise
	 */
sem_try_down:
	stmdb	sp!, {r4-r11,lr}
	mov	r1, r0			@ r1 = &sem_t
	mov	r2, #0			@ zero = locked

	ldrex	r0, [r1]		@ load sem->cur
	cmp	r0, #0			@ is it already locked?
	/* @@@ TODO: decrement for real semaphore */
	strexne	r0, r2, [r1]		@ try to claim if not
	cmpne	r0, #1			@ test for success
	beq	.Lfailed

	/* r0 = 0 if strex succeeds, so no need to set it manually */
	mov	r0, #0
	b	.Ldone

.Lfailed:
	mov	r0, #1

.Ldone:
	ldmia	sp!, {r4-r11,pc}
	.endfunc


	.global sem_up
	.func sem_up
	/*
	 * Entry: r0: (sem_t *)
	 */
sem_up:
	stmdb	sp!, {r4-r11,lr}
	mov	r1, r0			@ r1 = &sem_t
	mov	r2, #1			@ 1 = unlocked

.Lretry:
	ldrex	r0, [r1]		@ load sem->cur
	/* @@@ TODO: increment for real semaphore */
	strex	r0, r2, [r1]		@ unlock
	cmp	r0, #1			@ test for success
	beq	.Lretry			@ failed, so retry

	ldmia	sp!, {r4-r11,pc}
	.endfunc

