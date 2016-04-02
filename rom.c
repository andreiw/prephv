/*
 * OF emulation.
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

#include <rom.h>
#include <string.h>
#include <console.h>
#include <guest.h>
#include <mmu.h>

err_t
rom_call(eframe_t *frame)
{
	uint32_t *cia = (uint32_t*) frame->r3;
	LOG("OF call %s from 0x%lx",
	    (char *) (uintptr_t) *cia, frame->lr);

	if (!strcmp("finddevice", (char *) (uintptr_t) *cia)) {
		WARN("%s", (char *) (uintptr_t) *(cia + 3));
	} else if (!strcmp("exit", (char *) (uintptr_t) *cia)) {
		while(1);
	} else if (!strcmp("write", (char *) (uintptr_t) *cia)) {
		con_puts_len((char *) ra_2_ptr((uintptr_t) *(cia + 4)),
			     (uintptr_t) *(cia + 5));
	} else if (!strcmp("getprop", (char *) (uintptr_t) *cia)) {
		WARN("%s", (char *) (uintptr_t) *(cia + 4));
		frame->r3 = -1;
	} else if (!strcmp("claim", (char *) (uintptr_t) *cia)) {
		/* WARN("0x%lx", (char *) (uintptr_t) *(cia + 1)); */
		/* WARN("0x%lx", (char *) (uintptr_t) *(cia + 2)); */
		/* WARN("0x%lx", (char *) (uintptr_t) *(cia + 3)); */
		/* WARN("0x%lx", (char *) (uintptr_t) *(cia + 4)); */
		/* WARN("0x%lx", (char *) (uintptr_t) *(cia + 5)); */
		/* WARN("0x%lx", (char *) (uintptr_t) *(cia + 6)); */
		if ((uintptr_t) *(cia + 5) != 0) {
			guest->rom.claim_arena_ptr =
				ALIGN_UP(guest->rom.claim_arena_ptr,
					 (uintptr_t) *(cia + 5));
			
			BUG_ON(guest->rom.claim_arena_ptr >=
			       guest->rom.claim_arena_end, "claim arena overflow");
			
			*(cia + 6) = (uint32_t) guest->rom.claim_arena_ptr;
			guest->rom.claim_arena_ptr += (uintptr_t) *(cia + 4);
			
			BUG_ON(guest->rom.claim_arena_ptr >
			       guest->rom.claim_arena_end, "claim arena overflow");
			
			/* LOG("allocated at RA 0x%lx", foo); */
			frame->r3 = 0;
		} else {
			*(cia + 6) = *(cia + 3);
			/* LOG("claimed at RA 0x%lx", *(cia + 6)); */
			frame->r3 = 0;
		}
	}

	frame->hsrr0 = frame->lr;
	return ERR_NONE;
}

