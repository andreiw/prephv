/*
 * prephv layout.
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

#ifndef LAYOUT_H
#define LAYOUT_H

/*
 * 0x0000_0000_0000_0000 - 0x0000_0000_FFFF_FFFF : guest address space
 *
 * SLB slot 2:  SR#0
 *     slot 3:  SR#1
 *     slot 4:  SR#2
 *     slot 5:  SR#3
 *     slot 6:  SR#4
 *     slot 7:  SR#5
 *     slot 8:  SR#6
 *     slot 9:  SR#7
 *     slot 10: SR#8
 *     slot 11: SR#9
 *     slot 12: SR#A
 *     slot 13: SR#B
 *     slot 14: SR#C
 *     slot 15: SR#D
 *     slot 16: SR#E
 *     slot 17: SR#F
 *
 * ...4K pages, always.
 *
 * 0x8000_0000_0000_0000 - 0x8FFF_FFFF_FFFF_FFFF : prephv + direct RAM
 *
 * SLB slot 0: ESID 0x80_0000, 1TB segment, 16M pages.
 *
 * 0xC000_0000_0000_0000 - 0xC000_0000_0010_0000 : AIL exception vectors
 *
 * SLB slot 1: ESID 0xC0_0000, 256MB segment, 4K pages.
 *
 */

#define LAYOUT_VM_START 0x0000000000000000UL
#define LAYOUT_VM_END   0x0000000100000000UL
#define LAYOUT_HV_START 0x8000000000000000UL
#define LAYOUT_HV_END   0x9000000000000000UL

#endif /* LAYOUT_H */
