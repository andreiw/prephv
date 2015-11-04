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
#include <linkage.h>
#include <log.h>
#include <ppc.h>
#include <string.h>
#include <kpcr.h>
#include <time.h>
#include <exc.h>
#include <mem.h>

/*
 * ppc64le_hello has no concept of threads or per-thread exception stacks,
 * so when we switch into our atrophied user code, so we stash the
 * return state for when the user code quits.
 */
eframe_t ret_from_us;


void
exc_handler(eframe_t *frame)
{
	if ((frame->hsrr1 & MSR_RI) == 0) {
		/*
		 * Unrecoverable, possibly nested exception.
		 */
		goto bad;
	}

	/*
	 * Handle instruction storage faults within the HV address
	 * region (which is direct-mapped to 0). When the MMU is off,
	 * the translation happens automatically.
	 */
	if (frame->vec == EXC_ISI) {
		if (frame->hsrr0 >= HV_ASPACE &&
		    (frame->hsrr1 & SRR1_ISI_NOT_MAPPED) != 0) {
			mmu_map(frame->hsrr0 & PAGE_MASK_16M,
				ptr_2_ra((void *) (frame->hsrr0 & PAGE_MASK_16M)),
				PP_RWXX,
				PAGE_16M);
			exc_rfi(frame);
		}
	}

	/*
	 * Handle data storage faults within the HV address
	 * region (which is direct-mapped to 0). When the MMU is off,
	 * the translation happens automatically.
	 */
	if (frame->vec == EXC_DSI) {
		if (get_DAR() >= HV_ASPACE &&
		    (get_DSISR() & DSISR_NOT_MAPPED) != 0) {
			mmu_map(get_DAR() & PAGE_MASK_16M,
				ptr_2_ra((void *) (get_DAR() & PAGE_MASK_16M)),
				PP_RWXX,
				PAGE_16M);
			exc_rfi(frame);
		}
	}

	if (frame->vec == EXC_DEC) {
		time_handle();
		exc_rfi(frame);
	}

	if (frame->vec == EXC_HDEC) {
		set_HDEC(DEC_DISABLE);
		LOG("hypervisor decrementer, exception handler with MMU %s!",
		       (frame->hsrr1 & (MSR_IR | MSR_DR)) ? "on" : "off");
		exc_rfi(frame);
	}

	if (frame->vec == EXC_SC) {
		if (frame->r3 == 0xdead) {
			LOG("Triggering nested exception crash");
			asm volatile("sc");
		} else if (frame->r3 == 0x7e57) {
			eframe_t *ret_frame = (eframe_t *) frame->r4;
			uint64_t hsrr1_mask = ret_frame->hsrr1 & (MSR_HV | MSR_PR);
			/*
			 * A scheduler switching to an unpriviledged
			 * context needs to update the SP pointer used
			 * to handle kernel work on the context's behalf.
			 * In this lame example we piggy-back onto
			 * the stack used by main() (as it won't be running
			 * anyway until we return to it via
			 * test_syscall(0x1337, 0)), taking care to
			 * avoid clobbering the red and system zones.
			 */
			kpcr_get()->kern_sp = frame->r1 - ABI_STACK_PROTECTED_ZONE;
			ret_from_us = *frame;

			LOG("returning to ");
			if (hsrr1_mask == (MSR_HV | MSR_PR)) {
				LOG("HV user code");
			} else if (hsrr1_mask == MSR_HV) {
				LOG("HV priviledged code");
			} else if (hsrr1_mask == MSR_PR) {
				LOG("VM user code");
			} else if (hsrr1_mask == 0) {
				LOG("VM priviledged code");
			}
			exc_rfi((eframe_t *) frame->r4);
		} else if (frame->r3 == 0x1337) {
			LOG("returning back");
			exc_rfi(&ret_from_us);
		} else if (frame->r3 == 0xfeed) {
			frame->r3 = frame->r3 << 16 |
				frame->r4;
		} else if (frame->r3 == 0x1111) {
			LOG("%c", (char) frame->r4);
		} else if (frame->r3 == 0x1112) {
			LOG("0x%x",frame->r4);
		} else {
			LOG("unknown sc 0x%x",
			       frame->r3);
		}
		exc_rfi(frame);
	}

	if (frame->vec == EXC_ISEG ||
	    frame->vec == EXC_ISI) {
		LOG("Instruction MMU fault at 0x%x",
		       frame->hsrr0);
		goto bad;
	}

	if (frame->vec == EXC_DSEG ||
	    frame->vec == EXC_DSI) {
		LOG("Data MMU fault at 0x%x (DAR 0x%x, DSISR 0x%x)",
		       frame->hsrr0,
		       get_DAR(),
		       get_DSISR());
		goto bad;
	}

bad:
	LOG("Unrecoverable exception 0x%x\n"
	       "PC  = 0x%x\n"
	       "MSR = 0x%x",
	       frame->vec,
	       frame->hsrr0, frame->hsrr1);

#define D(x) LOG(#x " = 0x%x", frame->x)
	D(r0); D(r1); D(r2); D(r3); D(r4); D(r5);
	D(r6); D(r7); D(r8); D(r9); D(r10); D(r11);
	D(r12); D(r13); D(r14); D(r15); D(r16); D(r17);
	D(r18); D(r19); D(r20); D(r21); D(r22); D(r23);
	D(r24); D(r25); D(r26); D(r27); D(r28); D(r29);
	D(r30); D(r31); D(lr); D(ctr); D(xer); D(cr);
#undef D

	LOG("Hanging here...");
	while(1);
}


void
exc_restore_ee(exc_flags_t flags)
{
	mtmsrd(mfmsr() | (flags & MSR_EE), 1);
}


exc_flags_t
exc_disable_ee(void)
{
	exc_flags_t flags = mfmsr() & MSR_EE;

	mtmsrd(mfmsr() & ~MSR_EE, 1);
	return flags;
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
		LOG("OPAL claims no HILE supported, pretend to know better...");
		uint64_t hid0 = get_HID0();
		set_HID0(hid0 | HID0_HILE);
	}

	/*
	 * Force external interrupts to HV mode.
	 *
	 * Non-HV exceptions are LE (don't really need this here, but
	 * if there's MSR.HV=0 MSR.PR=1 code does an sc...). We call
	 * unpriviledged code with MSR.HV=1 MSR.PR=1.
	 *
	 * We also turn on AIL, so that when MMU is on, texceptions are taken
	 * with MMU  at EA 0xc000000000004000 instead of with MMU off at RA 0x0.
	 */
	set_LPCR((get_LPCR() & ~LPCR_LPES) | LPCR_ILE | LPCR_AIL0 | LPCR_AIL1);

	/*
	 * Exception stack.
	 */
	kpcr_get()->unrec_sp = (uint64_t) mem_alloc(PAGE_SIZE, PAGE_SIZE) +
		PAGE_SIZE  - sizeof(eframe_t);
	LOG("Unrecoverable exception stack top @ 0x%x",
	       kpcr_get()->unrec_sp);

	/*
	 * Copy vectors down.
	 */
	memcpy((void *) 0, &exc_base,
	       (uint64_t) &exc_end - (uint64_t) &exc_base);
	lwsync();
	flush_cache(0, (uint64_t) &exc_end - (uint64_t) &exc_base);

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
