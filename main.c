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
#include <libfdt.h>
#include <libfdt_internal.h>

#define HELLO_MAMBO "Hello Mambo!\n"
#define HELLO_OPAL "Hello OPAL!\n"

extern void * _start;
extern void * _bss_start;
extern void * _stack_start;
extern void * _end;


static void
dump_props(void *fdt, int node, int depth)
{
	const char *n;
	int i;
	uint32_t tag;
	int nextoffset;
	const struct fdt_property *prop;
	int offset = _fdt_check_node_offset(fdt, node);;

	if (offset < 0) {
		return;
	}

	do {
		tag = fdt_next_tag(fdt, offset, &nextoffset);
		if (tag == FDT_END) {
			return;
		}

		if (tag == FDT_PROP) {
			prop = _fdt_offset_ptr(fdt, offset);
			n = fdt_string(fdt, fdt32_to_cpu(prop->nameoff));

			for  (i = 0; i < depth; i++) {
				printk(" ");
			}
			printk("%s: 0x%x@0x%x\n", n,
			       fdt32_to_cpu(prop->len),
			       prop->data);
		}

		offset = nextoffset;
	} while ((tag != FDT_BEGIN_NODE) && (tag != FDT_END_NODE));
}



static void
dump_cpu(void *fdt)
{
	int cpu0_node;
	const uint32_t *be32_data;

	printk("CPU info:\n");
	cpu0_node = fdt_path_offset(fdt, "/cpus/cpu@0");
	if (cpu0_node < 0) {
		printk("CPU0 not found?\n");
		return;
	}

	be32_data = fdt_getprop(fdt, cpu0_node, "slb-size", NULL);
	if (be32_data != NULL) {
		printk("SLB size = 0x%x\n", be32_to_cpu(*be32_data));
	}

	be32_data = fdt_getprop(fdt, cpu0_node, "timebase-frequency", NULL);
	if (be32_data != NULL) {
		printk("TB freq = 0x%x\n", be32_to_cpu(*be32_data));
	}
}


static void
dump_nodes(void *fdt)
{
	int i;
	int numrsv;
	int offset;
	int depth;
	const char *n;

	if (fdt_check_header(fdt) != 0) {
		printk("Bad FDT\n");
		return;
	}

	numrsv = fdt_num_mem_rsv(fdt);
	printk("FDT version: %d\n", fdt_version(fdt));

	for (i = 0; i < numrsv; i++) {
		uint64_t addr, size;
		if (fdt_get_mem_rsv(fdt, i, &addr, &size) != 0) {
			break;
		}

		printk("/memreserve/ 0x%x 0x%x;\n",
		       addr, size);
	}

	offset = 0;
	depth = 0;
	do {
		offset = fdt_next_node(fdt, offset, &depth);
		if (offset < 0) {
			break;
		}

		n = fdt_get_name(fdt, offset, NULL);
		for  (i = 0; i < depth * 4; i++) {
			printk(" ");
		}

		if (n != NULL) {
			printk("<%s>\n", n);
		}

		dump_props(fdt, offset, depth * 4 + 2);
	} while(1);

	printk("Done dumping FDT\n");
	dump_cpu(fdt);
}


void
c_main(void *fdt)
{
	uint64_t len = cpu_to_be64(sizeof(HELLO_OPAL));
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

	dump_nodes(fdt);
}
