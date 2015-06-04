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

typedef uint64_t ra_t;
typedef uint64_t ea_t;
typedef uint64_t prot_t;


static inline ra_t
ptr_2_ra(void *addr) {
	ea_t ea = (ea_t) addr;

	BUG_ON((ea & HV_ASPACE) == 0, "must only pass HV addresses");
	return ea & ~HV_ASPACE;
}

typedef enum {
  PAGE_4K,
  PAGE_16M,
} page_size_t;

void mmu_init(length_t ram_size);
void mmu_enable(void);
void mmu_disable(void);
bool_t mmu_enabled(void);
void mmu_map(ea_t ea, ra_t ra, prot_t pp, page_size_t actual);
void mmu_unmap(ea_t ea, page_size_t actual);

#endif /* MMU_H */
