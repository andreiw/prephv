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
extern void *exc_end;
extern void *_unrec_stack_top;
extern void *_rec_stack_top;


void
exc_handler(eframe_t *frame)
{
	if ((frame->hsrr1 & MSR_RI) == 0) {
		/*
		 * Unrecoverable, possibly nested exception.
		 */
		goto bad;
	}

	if (frame->vec == EXC_DEC) {
		set_DEC(0x7fffffff);
		printk("decrementer!\n");
		exc_rfi(frame);
	}

	if (frame->vec == EXC_HDEC) {
		set_HDEC(0x7fffffff);
		printk("hypervisor decrementer!\n");
		exc_rfi(frame);
	}

	if (frame->vec == EXC_SC) {
		if (frame->r3 == 0xdead) {
			printk("Triggering nested exception crash\n");
			asm volatile("sc");
		}

		frame->r3 = frame->r3 << 16 | 0xface;
		exc_rfi(frame);
	}

bad:
	printk("Unrecoverable exception 0x%x\n"
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
exc_disable_ee(void)
{
	mtmsrd(mfmsr() & ~MSR_EE, 1);
}


void
exc_enable_ee(void)
{
	mtmsrd(mfmsr() | MSR_EE, 1);
}


void
exc_enable_hdec(void)
{
	set_LPCR(get_LPCR() | LPCR_HDICE);
}


void
exc_disable_hdec(void)
{
	set_LPCR(get_LPCR() & ~LPCR_HDICE);
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
	 * Force external interrupts to HV mode.
	 * Non-HV exceptions are LE (don't really need this here, but
	 * if there's ever user-space code doing an sc...).
	 * HV decrementer enabled.
	 */
	set_LPCR((get_LPCR() & ~LPCR_LPES) | LPCR_ILE);

	/*
	 * Exception stack.
	 */
	kpcr_get()->unrec_sp = (uint64_t) &_unrec_stack_top - sizeof(eframe_t);
	printk("Unrecoverable exception stack top @ 0x%x\n",
	       kpcr_get()->unrec_sp);
	kpcr_get()->rec_sp = (uint64_t) &_rec_stack_top;
	printk("Recoverable exception stack top @ 0x%x\n",
	       kpcr_get()->rec_sp);

	/*
	 * Copy vectors down.
	 */
	memcpy((void *) 0, &exc_base,
	       (uint64_t) &exc_end - (uint64_t) &exc_base);
	lwsync();

	/*
	 * Vectors expect HSPRG0 to contain the pointer to KPCR,
	 * HSPRG1 is scratch.
	 */
	set_HSPRG0((uint64_t) kpcr_get());

	/*
	 * Context is now recoverable.
	 */
	mtmsrd(MSR_RI, 1);
}
