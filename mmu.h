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

typedef uint64_t ra_t;
typedef uint64_t ea_t;
typedef uint64_t prot_t;

int mmu_init(uint64_t ram_size);
void mmu_enable(void);
void mmu_disable(void);
int mmu_enabled(void);
int mmu_map(ea_t ea, ra_t ra, prot_t pp);
int mmu_unmap(ea_t ea);
#endif /* MMU_H */
