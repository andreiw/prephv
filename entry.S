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
#include <ppc-defs.h>
#include <opal.h>
#include <kpcr.h>

/*
* Entering into this code we make the following assumptions:
*      The MMU is off, processor in HV mode, primary CPU enters
*      with device-tree in r3. We also get OPAL base in r8 and
*      entry in r9 for debugging purposes.
*/
_GLOBAL(_start)
	FIXUP_ENDIAN

/*
 * Set bit 63 (bit 0 in IBM-speak) in PC, so as to run
 * at the HV alias RM address.
 */
	bl	1f
1:	mflr	r1
	addi	r1, r1, 2f-1b
	li	r0, 1
	insrdi	r1, r0, 1, 0
	mtlr	r1
	blr
2:

	/*
	 * r3 - fdt
	 * r8 - OPAL base
	 * r9 - OPAL entry
	 */
	mr	r23, r3
	mr	r28, r8
	mr	r29, r9

	/*
	 * Setup the stack. This is r1.
	 */
	LOAD_IMM64(r1, _stack_top)
	subi	r1, r1, STACKFRAMEMIN

	/*
	 * Setup TOC for local calls. This must be in r2.
	 *
	 * You can also accomplish the same via:
	 * LOAD_IMM64(r3, .TOC.)
	 *
	 * ...but if you screw up your linker script, .TOC.
	 * and __toc_start won't match.
	 */
	LOAD_IMM64(r2, __toc_start)

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

	/*
	 * Point thread pointer to our toy per-cpu structure
	 * and set it up, storing OPAL base/entry and TOC away in
	 * it.
	 *
	 * Why this is imporant: we need to be able to restore
	 * our state inside opal_return. This is minimal since we
	 * run in real mode, but still. Also needed once we care
	 * to handle any exceptions.
	 */
	LOAD_IMM64(r13, kpcr)
	std	r28, KPCR(opal_base)
	std	r29, KPCR(opal_entry)
	std	r2, KPCR(toc)

	mr	r3, r23
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
