/*
 * Guest state.
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

#include <guest.h>
#include <log.h>
#include <mem.h>
#include <mmu.h>
#include <layout.h>

guest_t *guest;

err_t
guest_init(length_t ram_size)
{
	guest = mem_malloc(sizeof(guest_t));
	if (guest == NULL) {
		return ERR_NO_MEM;
	}

	BUG_ON(ram_size < MB(128), "too little RAM");

	guest->msr = 0;
	guest->ram_size = ram_size;
	guest->rom.claim_arena_start = guest->ram_size - MB(16);
	guest->rom.claim_arena_ptr = guest->rom.claim_arena_start;
	guest->rom.claim_arena_end = guest->ram_size;

	guest->ram = mem_memalign(MB(1), guest->ram_size);

	LOG("HV pointer to GRAM %p", guest->ram);
	LOG("Guest RAM at RA 0x%lx", ptr_2_ra(guest->ram));

	mmu_map_range(LAYOUT_VM_START,
		      LAYOUT_VM_START + guest->ram_size,
		      ptr_2_ra(guest->ram),
		      PP_RWRW,
		      PAGE_4K);

	return ERR_NONE;
}
