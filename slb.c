/*
 * SLB manipulation. From Linux kernel.
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
#include <console.h>
#include <kpcr.h>
#include <ppc.h>

/* 1T segments */
#define ESID_MASK_1T            0xffffff0000000000UL

/* Bits in the SLB ESID word */
#define SLB_ESID_V              (0x0000000008000000) /* valid */

/* Bits in the SLB VSID word */
#define SLB_VSID_SHIFT_1T       24
#define SLB_VSID_B_1T           (0x4000000000000000)
#define SLB_VSID_KP             (0x0000000000000400)


static uint64_t
slb_make_esid(uint64_t ea, uint64_t slot)
{
	/*
	 * 1TB.
	 */
	return (ea & ESID_MASK_1T) | SLB_ESID_V | slot;
}


static uint64_t
slb_make_vsid(uint64_t vsid)
{
	/*
	 * Supervisor, 1TB.
	 */
	return (vsid << SLB_VSID_SHIFT_1T) |
		SLB_VSID_B_1T |
		SLB_VSID_KP;
}


void
slb_init(void)
{
	uint64_t esid;
	uint64_t vsid;

	/*
	 * We don't do anything too crazy here, just set up 1
	 * segment where ESID (0) == VSID (0). Our ISI/DSI
	 * handlers will simply map the VA to RA 1:1, as long as
	 * VA is not beyond &_end.
	 */

	asm volatile("slbia" ::: "memory");
	isync();

	/*
	 * 1TB segment at EA=0 => VA=0. We use slot = 1,
	 * since slot 0 has special handling (not
	 * invalidated with tlbia).
	 */
	esid = slb_make_esid(0, 1);
	vsid = slb_make_vsid(0);
	asm volatile("slbmte %0, %1" ::
		     "r"(vsid), "r"(esid) : "memory");
	isync();
}


void
slb_dump(void)
{
	int entry;
	uint64_t esid;
	uint64_t vsid;

	for (entry = 0; entry < kpcr_get()->slb_size; ++entry) {
		asm volatile("slbmfee  %0,%1" : "=r" (esid) : "r" (entry));
		if (esid == 0) {
			/*
			 * Valid bit is clear along with everything else.
			 */
			continue;
		}

		asm volatile("slbmfev  %0,%1" : "=r" (vsid) : "r" (entry));
		printk("%d: E 0x%x V 0x%x\n", entry, esid, vsid);
	}
}
