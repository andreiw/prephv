/*
 * Entry point. Most of this code is derived from the Linux Kernel.
 *
 * Copyright (C) 2015 Andrei Warkentin <andrey.warkentin@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <asm-utils.h>
#include <ppc-regs.h>
#include <opal.h>

#define KPCR(name) (kpcr_ ## name - kpcr)(r13)

/*
* Entering into this code we make the following assumptions:
*      The MMU is off, processor in HV mode, primary CPU enters
*      with device-tree in r3. We also get OPAL base in r8 and
*      entry in r9 for debugging purposes.
*/
_GLOBAL(_start)
	FIXUP_ENDIAN

/*
 * Setup the stack. This is r1.
 */
	LOAD_IMM64(r1, _stack_top)
	addi r1, r1, -STACKFRAMEMIN

/*
 * Setup TOC for local calls. This must be in r2.
 */
	LOAD_IMM64(r2, .TOC.)

/*
 * Point thread pointer to our toy per-cpu structure
 * and set it up, storing OPAL base/entry and TOC away in it.
 *
 * Why this is imporant: we need to be able to restore
 * our state inside opal_return. This is minimal since we
 * run in real mode, but still. Also needed once we care
 * to handle any exceptions.
 */
	LOAD_IMM64(r13, kpcr)
	std	r8, KPCR(opal_base)
	std	r9, KPCR(opal_entry)
	std	r2, KPCR(toc)

/*
 * Clear BSS (including a 0-sized BSS).
 */
	li	r0, 0
	LOAD_ADDR(r8, _bss_start)
	LOAD_ADDR(r9, _bss_end)
	subf	r9, r8, r9
	srdi	r9, r9, 3
	addi	r9, r9, 1
	mtctr	r9
	bdz	2f
1:	std	r0, 0(r8)
	addi	r8, r8, 8
	bdnz	1b
2:

	bl	c_main
	/*
	 * ABI mandates that a branch and link is followed by a
	 * nop to handle calls to functions outside of the linked
	 * module. This is what code generated with gcc would look
	 * like, so we keep it.
	 */
	ori	r0, r0, 0
	/*
	 * Stop sim.
	 */
	attn

/*
 * Use simulator callthru (i.e. semihosting) facilities
 * to print a string.
 *
 * r3 - buffer
 * r4 - len
 */
_GLOBAL(mambo_write)
#define SIM_WRITE_CONSOLE_CODE 0
	li	r6, 0
	mr	r5, r4
	mr	r4, r3
	li	r3, SIM_WRITE_CONSOLE_CODE
	.long	0x000eaeb0
	blr

/*
 * Perform a call to the OpenPower Abstraction Layer
 * from little-endian real-mode. virt-mode unsupported.
 */
#define OPAL_CALL(name, token)				\
	_GLOBAL(name);					\
	li	r0, token;				\
	mflr	r12;					\
	std	r12,STK_LR(r1);				\
	ld	r2, KPCR(toc);				\
	LOAD_ADDR(r12, opal_return);			\
	mtlr	r12;					\
	/*
	 * OPAL calls done in big-endian call, so we	\
	 * will return from interrupt into big-endian	\
	 * mode running the OPAL entry.			\
	 */						\
	mfmsr	r12;					\
	li	r11, MSR_LE;				\
	andc	r12, r12, r11;				\
	mtspr	SPRN_HSRR1, r12;			\
	ld	r2, KPCR(opal_base);			\
	ld	r12, KPCR(opal_entry);			\
	mtspr	SPRN_HSRR0, r12;			\
	/*						\
	 * OPAL preserves r1, r13-r31. r3-r11 are	\
	 * arguments.					\
	 */						\
	hrfid
_GLOBAL(opal_return)
	FIXUP_ENDIAN
	ld	r2, KPCR(toc)
	ld	r12,STK_LR(r1)
	mtlr	r12
	blr

OPAL_CALL(opal_write, OPAL_CONSOLE_WRITE)
OPAL_CALL(opal_read, OPAL_CONSOLE_READ)

/*
 * Toy per-cpu structure (for only 1 CPU).
 */
kpcr:
kpcr_toc:
	.llong 0
kpcr_opal_base:
	.llong 0
kpcr_opal_entry:
	.llong 0
