/*
 * Exception handlers.
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

.p2align 8
.globl exc_base
exc_base:
	std	r0, eframe_vec(r1)
	mfspr	r0, SPRN_HSPRG1
	std	r0, eframe_r1(r1)
	std	r2, eframe_r2(r1)
	std	r3, eframe_r3(r1)
	std	r4, eframe_r4(r1)
	std	r5, eframe_r5(r1)
	std	r6, eframe_r6(r1)
	std	r7, eframe_r7(r1)
	std	r8, eframe_r8(r1)
	std	r9, eframe_r9(r1)
	std	r10, eframe_r10(r1)
	std	r11, eframe_r11(r1)
	std	r12, eframe_r12(r1)
	std	r13, eframe_r13(r1)
	std	r14, eframe_r14(r1)
	std	r15, eframe_r15(r1)
	std	r16, eframe_r16(r1)
	std	r17, eframe_r17(r1)
	std	r18, eframe_r18(r1)
	std	r19, eframe_r19(r1)
	std	r20, eframe_r20(r1)
	std	r21, eframe_r21(r1)
	std	r22, eframe_r22(r1)
	std	r23, eframe_r23(r1)
	std	r24, eframe_r24(r1)
	std	r25, eframe_r25(r1)
	std	r26, eframe_r26(r1)
	std	r27, eframe_r27(r1)
	std	r28, eframe_r28(r1)
	std	r29, eframe_r29(r1)
	std	r30, eframe_r30(r1)
	std	r31, eframe_r31(r1)
	mflr	r0
	std	r0, eframe_lr(r1)
	mfctr	r0
	std	r0, eframe_ctr(r1)
	mfxer	r0
	std	r0, eframe_xer(r1)
	mfcr	r0
	std	r0, eframe_cr(r1)
	mfspr	r13, SPRN_HSPRG0
	ld	r2, KPCR(toc)
	mr	r3, r1
	subi	r1, r1, STACKFRAMEMIN
	ld	r0, KPCR(exc_handler)
	mtctr	r0
	bctr

#define DEF_EXC(name)				\
	.org exc_base + EXC_##name;		\
	.globl exc_##name;			\
	exc_##name:				\
	mtspr	SPRN_HSPRG1, r1;		\
	mfspr	r1, SPRN_HSPRG0;		\
	ld	r1, KPCR_R(exc_r1, r1);		\
	subi	r1, r1, eframe_sizeof;		\
	std	r0, eframe_r0(r1);		\
	li	r0, EXC_##name;			\
	b exc_base;				\

DEF_EXC(RESET)
DEF_EXC(MC)
DEF_EXC(DSI)
DEF_EXC(DSEG)
DEF_EXC(ISI)
DEF_EXC(ISEG)
DEF_EXC(EXT)
DEF_EXC(ALIGN)
DEF_EXC(PROG)
DEF_EXC(FPU)
DEF_EXC(DEC)
DEF_EXC(HDEC)
DEF_EXC(DOOR)
DEF_EXC(RESV)
DEF_EXC(SC)
DEF_EXC(TRACE)
DEF_EXC(HDSI)
DEF_EXC(HISI)
DEF_EXC(HEA)
DEF_EXC(HMAINT)
DEF_EXC(HDOOR)
DEF_EXC(RESV1)
DEF_EXC(RESV2)
DEF_EXC(IMPL)
DEF_EXC(PMU)
DEF_EXC(VECUN)
DEF_EXC(VSX)
DEF_EXC(FAC)
DEF_EXC(HFAC)
.org exc_base + EXC_TABLE_END
.globl exc_table_end
exc_table_end:

_GLOBAL(exc_rfi)
	ld	r0, eframe_hsrr1(r3)
	mtspr	SPRN_HSRR1, r0
	ld	r0, eframe_hsrr0(r3)
	mtspr	SPRN_HSRR0, r0
	ld	r0, eframe_cr(r3)
	mtcr	r0
	ld	r0, eframe_xer(r3)
	mtxer	r0
	ld	r0, eframe_ctr(r3)
	mtctr	r0
	ld	r0, eframe_lr(r3)
	mtlr	r0
	ld	r31, eframe_r31(r3)
	ld	r30, eframe_r30(r3)
	ld	r29, eframe_r29(r3)
	ld	r28, eframe_r28(r3)
	ld	r27, eframe_r27(r3)
	ld	r26, eframe_r26(r3)
	ld	r25, eframe_r25(r3)
	ld	r24, eframe_r24(r3)
	ld	r23, eframe_r23(r3)
	ld	r22, eframe_r22(r3)
	ld	r21, eframe_r21(r3)
	ld	r20, eframe_r20(r3)
	ld	r19, eframe_r19(r3)
	ld	r18, eframe_r18(r3)
	ld	r17, eframe_r17(r3)
	ld	r16, eframe_r16(r3)
	ld	r15, eframe_r15(r3)
	ld	r14, eframe_r14(r3)
	ld	r13, eframe_r13(r3)
	ld	r12, eframe_r12(r3)
	ld	r11, eframe_r11(r3)
	ld	r10, eframe_r10(r3)
	ld	r9, eframe_r9(r3)
	ld	r8, eframe_r8(r3)
	ld	r7, eframe_r7(r3)
	ld	r6, eframe_r6(r3)
	ld	r5, eframe_r5(r3)
	ld	r4, eframe_r4(r3)
	ld	r2, eframe_r2(r3)
	ld	r1, eframe_r1(r3)
	ld	r0, eframe_r0(r3)
	ld	r3, eframe_r3(r3)
	hrfid
