/*
 * SLB/HTAB management.
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

#ifndef MMU_H
#define MMU_H

#include <types.h>
#include <assert.h>

typedef uint64_t prot_t;

static inline ra_t
ptr_2_ra(void *addr) {
	ea_t ea = (ea_t) addr;

	/*
	 * If we're in real mode, then bit 63 of EA is aliased to 0.
	 * If MMU is enabled, then we map EA 1TB @ 0x8000000000000000 to RA 0x0.
	 *
	 * I call EAs with bit 63 set HV addresses.
	 *
	 * Kernel code is linked to addresses in 0x8000000000000000 and all memory
	 * is always accessed using HV addresses.
	 */
	BUG_ON(ea < HV_ASPACE, "ea 0x%x is not an HV address", ea);
	return ea & ~HV_ASPACE;
}


static inline void *
ra_2_ptr(ra_t ra) {
	/*
	 * HV_ASPACE is a feature of EAs, not RA. But
	 * catch EAs being passed in.
	 */
	BUG_ON(ra >= HV_ASPACE, "ra 0x%x looks like an EA (HV address)", ra);
	return (void *) (ra | HV_ASPACE);
}

typedef enum {
  SEG_256M,
  SEG_1T,
} seg_size_t;

typedef enum {
  PAGE_4K,
  PAGE_16M,
} page_size_t;

void mmu_init(length_t ram_size);
void mmu_enable(void);
void mmu_disable(void);
bool_t mmu_enabled(void);
void mmu_map(ea_t ea, ra_t ra, prot_t pp, page_size_t actual);

err_t mmu_guest_fault_seg(ea_t ea);

void
mmu_map_range(ea_t ea_start,
	      ea_t ea_end,
	      ra_t ra_start,
	      prot_t prot,
	      page_size_t actual);

void mmu_unmap(ea_t ea, page_size_t actual);

#endif /* MMU_H */
