/*
 * Exception handling.
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

#include <types.h>
#include <opal.h>
#include <console.h>
#include <ppc.h>
#include <string.h>
#include <kpcr.h>
#include <exc.h>

extern void *exc_base;
extern void *_exc_stack_top;


void
exc_handler(eframe_t *frame)
{
	switch (frame->vec) {
	case EXC_HDEC:
	case EXC_HDSI:
	case EXC_HISI:
	case EXC_HEA:
	case EXC_HMAINT:
	case EXC_HDOOR:
	case EXC_HFAC:
		frame->hsrr0 = get_HSRR0();
		frame->hsrr1 = get_HSRR1();
	default:
		/*
		 * Saved MSR/PC get stored in
		 * in different registers depending
		 * on the exception type. But we'll
		 * always use hrfid to return, and thus
		 * will always use HSRR0/HSRR1.
		 */
		frame->hsrr0 = get_SRR0();
		frame->hsrr1 = get_SRR1();
	}

	if (frame->vec == EXC_SC) {
		frame->r3 = frame->r3 << 16 | 0xface;
		exc_rfi(frame);
	}

	printk("Exception 0x%x\n"
	       "PC  = 0x%x\n"
	       "MSR = 0x%x\n",
	       frame->vec,
	       frame->hsrr0, frame->hsrr1);

#define D(x) printk(#x " = 0x%x\n", frame->x)
	D(r0); D(r1); D(r2); D(r3); D(r4); D(r5);
	D(r6); D(r7); D(r8); D(r9); D(r10); D(r11);
	D(r12); D(r13); D(r14); D(r15); D(r16); D(r17);
	D(r18); D(r19); D(r20); D(r21); D(r22); D(r23);
	D(r24); D(r25); D(r26); D(r27); D(r28); D(r29);
	D(r30); D(r31); D(lr); D(ctr); D(xer); D(cr);
#undef D

	printk("Hanging here...\n");
	while(1);
}


void
exc_init(void)
{

	/*
	 * MSR.ILE controls supervisor exception endianness. OPAL
	 * takes care of setting the hypervisor exception endianness
	 * bit in an implementation-neutral fashion. Mambo systemsim
	 * doesn't seem to report a PVR version that Skiboot recognises
	 * as supporting HID0.HILE, so we'll manually set it until
	 * the skiboot patch goes in.
	 */
	if (opal_reinit_cpus(OPAL_REINIT_CPUS_HILE_LE) != OPAL_SUCCESS) {
		printk("OPAL claims no HILE supported, pretend to know better...\n");
		uint64_t hid0 = get_HID0();
		set_HID0(hid0 | HID0_HILE);
	}

	/*
	 * Exception stack.
	 */
	kpcr_get()->exc_r1 = (uint64_t) &_exc_stack_top - STACKFRAMEMIN;
	kpcr_get()->exc_handler = (uint64_t) exc_handler;
	printk("Exception stack top @ 0x%x\n", kpcr_get()->exc_r1);
	printk("Exception handler @ 0x%x\n", kpcr_get()->exc_handler);

	/*
	 * Copy vectors down.
	 */
	memcpy((void *) 0, &exc_base, EXC_TABLE_END);
	lwsync();
}
