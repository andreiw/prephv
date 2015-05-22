/*
 * C offsets for assembler code.
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

#include <kpcr.h>
#include <stddef.h>
#include <exc.h>

#define DEFINE(sym, val) \
	asm volatile("\n->" #sym " %0 " #val : : "i" (val))

#define OFFSET(sym, str, mem)			\
	DEFINE(sym, offsetof(struct str, mem))

void
unused(void)
{

	/* KPCR offsets. */
	OFFSET(kpcr_toc, kpcr_s, toc);
	OFFSET(kpcr_opal_base, kpcr_s, opal_base);
	OFFSET(kpcr_opal_entry, kpcr_s, opal_entry);
	OFFSET(kpcr_slb_size, kpcr_s, slb_size);
	OFFSET(kpcr_unrec_sp, kpcr_s, unrec_sp);
	OFFSET(kpcr_rec_sp, kpcr_s, rec_sp);

	/* Exception frame stuff. */
	DEFINE(eframe_sizeof, sizeof(eframe_t));
	OFFSET(eframe_vec, eframe_s, vec);
	OFFSET(eframe_r0, eframe_s, r0);
	OFFSET(eframe_r1, eframe_s, r1);
	OFFSET(eframe_r2, eframe_s, r2);
	OFFSET(eframe_r3, eframe_s, r3);
	OFFSET(eframe_r4, eframe_s, r4);
	OFFSET(eframe_r5, eframe_s, r5);
	OFFSET(eframe_r6, eframe_s, r6);
	OFFSET(eframe_r7, eframe_s, r7);
	OFFSET(eframe_r8, eframe_s, r8);
	OFFSET(eframe_r9, eframe_s, r9);
	OFFSET(eframe_r10, eframe_s, r10);
	OFFSET(eframe_r11, eframe_s, r11);
	OFFSET(eframe_r12, eframe_s, r12);
	OFFSET(eframe_r13, eframe_s, r13);
	OFFSET(eframe_r14, eframe_s, r14);
	OFFSET(eframe_r15, eframe_s, r15);
	OFFSET(eframe_r16, eframe_s, r16);
	OFFSET(eframe_r17, eframe_s, r17);
	OFFSET(eframe_r18, eframe_s, r18);
	OFFSET(eframe_r19, eframe_s, r19);
	OFFSET(eframe_r20, eframe_s, r20);
	OFFSET(eframe_r21, eframe_s, r21);
	OFFSET(eframe_r22, eframe_s, r22);
	OFFSET(eframe_r23, eframe_s, r23);
	OFFSET(eframe_r24, eframe_s, r24);
	OFFSET(eframe_r25, eframe_s, r25);
	OFFSET(eframe_r26, eframe_s, r26);
	OFFSET(eframe_r27, eframe_s, r27);
	OFFSET(eframe_r28, eframe_s, r28);
	OFFSET(eframe_r29, eframe_s, r29);
	OFFSET(eframe_r30, eframe_s, r30);
	OFFSET(eframe_r31, eframe_s, r31);
	OFFSET(eframe_lr, eframe_s, lr);
	OFFSET(eframe_ctr, eframe_s, ctr);
	OFFSET(eframe_xer, eframe_s, xer);
	OFFSET(eframe_cr, eframe_s, cr);
	OFFSET(eframe_hsrr0, eframe_s, hsrr0);
	OFFSET(eframe_hsrr1, eframe_s, hsrr1);
}
