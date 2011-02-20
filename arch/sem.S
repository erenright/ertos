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

