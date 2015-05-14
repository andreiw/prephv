/*
 * Simple PPC64LE freestanding "Hello, World" ``kernel'', meant
 * to be booted via skiboot or another OPAL firmware on the
 * "mambo" open-power sim.
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
#include <endian.h>
#include <console.h>
#include <kpcr.h>

#define HELLO_MAMBO "Hello Mambo!\n"
#define HELLO_OPAL "Hello OPAL!\n"

extern void * _start;
extern void * _bss_start;
extern void * _stack_start;
extern void * _end;


void
c_main(void *fdt)
{
	u64 len = cpu_to_be64(sizeof(HELLO_OPAL));
	/*
	 * Write using sim interface (simpler).
	 */
	mambo_write(HELLO_MAMBO, sizeof(HELLO_MAMBO));

	/*
	 * Write using firmware interface.
	 */
	opal_write(OPAL_TERMINAL_0, &len, HELLO_OPAL);

	/*
	 * Some info.
	 */
	printk("_start = %p\n", &_start);
	printk("_bss   = %p\n", &_bss_start);
	printk("_stack = %p\n", &_stack_start);
	printk("_end   = %p\n", &_end);
	printk("KPCR   = %p\n", kpcr_get());
	printk("OPAL   = %p\n", kpcr_get()->opal_base);
	printk("FDT    = %p\n", fdt);
}
