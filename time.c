/*
 * Timebase and decrementer support.
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

#include <ppc.h>
#include <time.h>
#include <console.h>

void
time_delay(uint64_t delay)
{
	uint64_t now = mftb();
	printk("Delaying for %u.%us\n",
	       delay / kpcr_get()->tb_freq,
	       delay % kpcr_get()->tb_freq);
	while ((mftb() - now) < delay) {
		cpu_relax();
		printk("Now = %u.%u s\n",
		       (mftb() - now) / kpcr_get()->tb_freq,
		       (mftb() - now) % kpcr_get()->tb_freq);
	}
	printk("Done!\n");
}
