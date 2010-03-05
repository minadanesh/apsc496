/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You
 * may not reproduce, modify or distribute this software except in
 * compliance with the License. You may obtain a copy of the License
 * at: http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 * This file may contain contributions from others, either as
 * contributors under the License or as licensors under other terms.
 * Please review this entire file for other proprietary rights or license
 * notices, as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */


/*
 * Reset code for TI OMAP3530 
 * the main code execute in SRAM.
 */

	.text
	.align	2
	.globl	_start

_start:
	
	/*
	 * Set the cpu to SVC32 mode
	 */
	mrs		r0, cpsr
	bic		r0, r0, #0x1f
	orr		r0, r0, #0xd3
	msr		cpsr, r0

	/*
	 * Invalidate L1 I/D
	 */
        mov	r0, #0                 /* set up for MCR */
        mcr	p15, 0, r0, c8, c7, 0  /* invalidate TLBs */
        mcr	p15, 0, r0, c7, c5, 0  /* invalidate icache */

	/*
	 * disable MMU stuff and caches
	 */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x00002000	@ clear bits 13 (--V-)
	bic	r0, r0, #0x00000007	@ clear bits 2:0 (-CAM)
	orr	r0, r0, #0x00000002	@ set bit 1 (--A-) Align
	orr	r0, r0, #0x00000800	@ set bit 11 (Z---) BTB
	mcr	p15, 0, r0, c1, c0, 0

	/*
	 * Set stack pointer
	 */
	ldr		sp, =0x4020A000

	bl		main

	/*
	 * In case return from main
	 */
	b		_start

/*
 *	v7_flush_dcache_all()
 *
 *	Flush the whole D-cache.
 *
 *	Corrupted registers: r0-r5, r7, r9-r11
 *
 */
	.align 5
.global v7_flush_dcache_all
v7_flush_dcache_all:
	stmfd	r13!, {r0-r5, r7, r9-r12,r14}

	mov	r7, r0				@ take a backup of device type
	cmp	r0, #0x3			@ check if the device type is GP
	moveq r12, #0x1				@ set up to invalide L2	
smi: 	.word 0x01600070			@ Call SMI monitor (smieq)
	cmp	r7, #0x3			@ compare again in case its lost
	beq	finished_inval			@ if GP device, inval done above

	mrc	p15, 1, r0, c0, c0, 1		@ read clidr
	ands	r3, r0, #0x7000000		@ extract loc from clidr
	mov	r3, r3, lsr #23			@ left align loc bit field
	beq	finished_inval			@ if loc is 0, then no need to clean
	mov	r10, #0				@ start clean at cache level 0
inval_loop1:
	add	r2, r10, r10, lsr #1		@ work out 3x current cache level
	mov	r1, r0, lsr r2			@ extract cache type bits from clidr
	and	r1, r1, #7			@ mask of the bits for current cache only
	cmp	r1, #2				@ see what cache we have at this level
	blt	skip_inval				@ skip if no cache, or just i-cache
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	isb					@ isb to sych the new cssr&csidr
	mrc	p15, 1, r1, c0, c0, 0		@ read the new csidr
	and	r2, r1, #7			@ extract the length of the cache lines
	add	r2, r2, #4			@ add 4 (line length offset)
	ldr	r4, =0x3ff
	ands	r4, r4, r1, lsr #3		@ find maximum number on the way size
	clz	r5, r4				@ find bit position of way size increment
	ldr	r7, =0x7fff
	ands	r7, r7, r1, lsr #13		@ extract max number of the index size
inval_loop2:
	mov	r9, r4				@ create working copy of max way size
inval_loop3:
	orr	r11, r10, r9, lsl r5		@ factor way and cache number into r11
	orr	r11, r11, r7, lsl r2		@ factor index number into r11
	mcr	p15, 0, r11, c7, c6, 2		@ invalidate by set/way
	subs	r9, r9, #1			@ decrement the way
	bge	inval_loop3
	subs	r7, r7, #1			@ decrement the index
	bge	inval_loop2
skip_inval:
	add	r10, r10, #2			@ increment cache number
	cmp	r3, r10
	bgt	inval_loop1
finished_inval:
	mov	r10, #0				@ swith back to cache level 0
	mcr	p15, 2, r10, c0, c0, 0		@ select current cache level in cssr
	isb
	
	ldmfd	r13!, {r0-r5, r7, r9-r12,pc}

_main: .word main

