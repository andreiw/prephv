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

extern void *exc_base;


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
	 * Copy vectors down.
	 */
	memcpy((void *) 0, &exc_base, EXC_TABLE_END);
	lwsync();
}
