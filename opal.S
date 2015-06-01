/*
 * OPAL support.
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
#include <ppc-defs.h>
#include <opal.h>
#include <kpcr.h>

/*
 * Perform a call to the OpenPower Abstraction Layer
 * from little-endian mode (virt or real).
 */
#define OPAL_CALL(name, token)				\
	_GLOBAL(name);					\
	li	r0, token;				\
	b	opal_call_common;
opal_call_common:
	mflr	r12
	std	r12,STK_LR(r1)
	mfcr	r12;
	std	r12,STK_CR(r1)
	/*
	 * We'll use r14 to pass current MSR to opal_return,
	 * so save clobbered r14 on stack.
	 */
	std	r14,-8(r1)
	LOAD_ADDR(r12, opal_return)
	mtlr	r12
	/*
	 * OPAL calls done in big-endian call, so we
	 * will return from interrupt into big-endian
	 * mode running the OPAL entry.
	 */
	mfmsr	r14;
	li	r11, MSR_LE;
	/*
	 * MMU has to be off, as must external interrupts.
	 */
	ori	r11, r11, MSR_EE;
	ori	r11, r11, MSR_IR;
	ori	r11, r11, MSR_DR;
	andc	r12, r14, r11;
	mtspr	SPRN_HSRR1, r12;
	ld	r2, KPCR(opal_base);
	ld	r12, KPCR(opal_entry);
	mtspr	SPRN_HSRR0, r12;
	/*
	 * OPAL preserves r1, r13-r31. r3-r11 are
	 * arguments.
	 */
	hrfid
_GLOBAL(opal_return)
	/*
	 * Enter here in BE with r13 = KPCR, r14 = old MSR.
	 * Stack contains CR/LR in ABI slots, and clobbered r14
	 * value at -8.
	 *
	 * We must restore MSR before touching any memory, because
	 * virt-mode might need to be on.
	 */
	FIXUP_ENDIAN
	mtmsr	r14
	ld	r2, KPCR(toc)
	ld	r12,STK_CR(r1)
	mtcr	r12
	ld	r12,STK_LR(r1)
	ld	r14,-8(r1)
	mtlr	r12
	blr

OPAL_CALL(opal_write, OPAL_CONSOLE_WRITE)
OPAL_CALL(opal_read, OPAL_CONSOLE_READ)
OPAL_CALL(opal_reinit_cpus, OPAL_REINIT_CPUS)
