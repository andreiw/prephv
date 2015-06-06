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
#include <linkage.h>
#include <mambo.h>
#include <console.h>
#include <kpcr.h>
#include <opal.h>
#include <libfdt.h>
#include <libfdt_internal.h>
#include <exc.h>
#include <ppc.h>
#include <time.h>
#include <mmu.h>
#include <mem.h>

#define HELLO_MAMBO "Hello Mambo!\n"
#define HELLO_OPAL "Hello OPAL!\n"

kpcr_t kpcr;


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

			if (fdt32_to_cpu(prop->len) == 0) {
				printk("%s: TRUE\n", n);
			} else if (!strcmp(n, "compatible") ||
				   !strcmp(n, "bootargs") ||
				   !strcmp(n, "linux,stdout-path") ||
				   !strcmp(n, "epapr-version") ||
				   !strcmp(n, "model") ||
				   !strcmp(n, "device_type")
				) {
				printk("%s: %s\n", n, prop->data);
			} else if (!strcmp(n, "#address-cells") ||
				   !strcmp(n, "#size-cells") ||
				   !strcmp(n, "#bytes") ||
				   !strcmp(n, "l2-cache-size") ||
				   !strcmp(n, "slb-size") ||
				   !strcmp(n, "timebase-frequency") ||
				   !strcmp(n, "i-cache-size") ||
				   !strcmp(n, "i-cache-sets") ||
				   !strcmp(n, "i-cache-line-size") ||
				   !strcmp(n, "i-cache-block-size") ||
				   !strcmp(n, "d-cache-size") ||
				   !strcmp(n, "d-cache-sets") ||
				   !strcmp(n, "d-cache-line-size") ||
				   !strcmp(n, "d-cache-block-size") ||
				   !strcmp(n, "linux,phandle") ||
				   !strcmp(n, "phandle")) {
				printk("%s: 0x%x\n", n,
				       be32_to_cpu(*(uint32_t *) prop->data));
			} else {
				printk("%s: 0x%x@0x%x\n", n,
				       fdt32_to_cpu(prop->len),
				       prop->data);
			}
		}

		offset = nextoffset;
	} while ((tag != FDT_BEGIN_NODE) && (tag != FDT_END_NODE));
}


static void
cpu_init(void *fdt)
{
	int cpu0_node;
	const uint32_t *be32_data;
	unsigned int slb_size;
	unsigned int tb_freq;

	cpu0_node = fdt_node_offset_by_dtype(fdt, -1, "cpu");
	if (cpu0_node < 0) {
		printk("CPU0 not found?\n");
		return;
	}

	be32_data = fdt_getprop(fdt, cpu0_node, "slb-size", NULL);
	if (be32_data != NULL) {
		slb_size = be32_to_cpu(*be32_data);
	} else {
		printk("Assuming default SLB size\n");
		slb_size = 32;
	}
	printk("SLB size = 0x%x\n", slb_size);
	kpcr_get()->slb_size = slb_size;

	be32_data = fdt_getprop(fdt, cpu0_node, "timebase-frequency", NULL);
	if (be32_data != NULL) {
		tb_freq = be32_to_cpu(*be32_data);
	} else {
		printk("Assuming default TB frequency\n");
		tb_freq = 512000000;
	}
	printk("TB freq = %u\n", tb_freq);
	kpcr_get()->tb_freq = tb_freq;

	exc_init();
	mmu_init((ea_t) &_end - (ea_t) &_start);
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
		ra_t addr;
		length_t size;
		if (fdt_get_mem_rsv(fdt, i, &addr, &size) != 0) {
			break;
		}

		printk("/memreserve/ 0x%x 0x%x;\n",
		       addr, size);
	}

	offset = 0;
	depth = 0;
	do {
		n = fdt_get_name(fdt, offset, NULL);
		for  (i = 0; i < depth * 4; i++) {
			printk(" ");
		}

		if (n != NULL) {
			printk("<%s>\n", n);
		}

		dump_props(fdt, offset, depth * 4 + 2);

		offset = fdt_next_node(fdt, offset, &depth);
		if (offset < 0) {
			break;
		}
	} while(1);
}


static uint64_t
test_syscall(uint64_t param1, uint64_t param2)
{
	register uint64_t r3 __asm__ ("r3") = param1;
	register uint64_t r4 __asm__ ("r4") = param2;
	asm volatile("sc" : "=r" (r3) : "r" (r3), "r" (r4));
	return r3;
}


static void
test_u(void)
{
	static void *upage = NULL;
	int en = mmu_enabled();
	eframe_t uframe;

	/*
	 * 1TB - 4K.
	 */
	ea_t ea = TB(1) - PAGE_SIZE;

	if (upage == NULL) {
		upage = mem_alloc(PAGE_SIZE, PAGE_SIZE);
	}

	if (!en) {
		mmu_enable();
	}

	/*
	 * Grab a page. Needs to be mapped with access from
	 * unpriviledged mode. We will use both to contain
	 * code to run and the stack.
	 */
	mmu_map(ea, ptr_2_ra(upage), PP_RWRW, PAGE_4K);
	memcpy((void *) ea, (void *) test_syscall, (uint64_t) &test_u -
	       (uint64_t) &test_syscall);
	lwsync();
	flush_cache(ea, PAGE_SIZE);

	/*
	 * User code does test_syscall(0x1337, 0), which simply returns
	 * to kernel state.
	 */
	uframe.r1 = ea + PAGE_SIZE - STACKFRAMESIZE;
	uframe.r3 = 0x1337;
	uframe.r4 = 0;
	uframe.hsrr0 = ea;
	uframe.hsrr1 = mfmsr() | MSR_PR;

	/*
	 * Force switch into user code. The exception handler stashes
	 * the kernel state behind in a global (sigh), which is magically
	 * restored on a test_syscall(0x1337, 0). The things we do
	 * to avoid writing an actual scheduler.
	 */
	test_syscall(0x7e57, (uint64_t) &uframe);

	/*
	 * We return here.
	 */
	mmu_unmap(ea, PAGE_4K);

	if (!en) {
		mmu_disable();
	}
}


static void
test_mmu_16mb(void)
{
	int i;
	bool_t good;

	/*
	 * 1TB - 16MB.
	 */
	uint64_t *ea = (uint64_t *) (TB(1) - MB(16));
	uint64_t *ea2 = (uint64_t *) (TB(1) - MB(32));
	int en = mmu_enabled();
	static uint64_t *p1 = NULL;
	static uint64_t *p2 = NULL;

	if (p1 == NULL) {
		p1 = mem_alloc(MB(16), MB(16));
	}

	if (p2 == NULL) {
		p2 = mem_alloc(MB(16), MB(16));
	}

	if (!en) {
		mmu_enable();
	}

	printk("mapping two EAs to same RA - on a sim this will take a while...\n");
	mmu_map((ea_t) ea, ptr_2_ra(p1), PP_RWXX, PAGE_16M);
	printk("mapped 0x%x to 0x%x as 16M\n", ea, p1);
	mmu_map((ea_t) ea2, ptr_2_ra(p1), PP_RWXX, PAGE_16M);
	printk("mapped 0x%x to 0x%x as 16M\n", ea2, p1);
	for (i = 0; i < (PAGE_SIZE * 2 / sizeof(uint64_t)); i++) {
		ea[i] = (uint64_t) &p1[i];
	}
	printk("prep completed\n");
	good = TRUE;
	for (i = 0; i < (PAGE_SIZE * 2 / sizeof(uint64_t)); i++) {
		if (ea2[i] != (uint64_t) &p1[i]) {
			good = FALSE;
			break;
		}
	}
	printk("16M mappings %swork\n", !good ? "don't " : "");
	if (!good) {
		goto out;
	}

	printk("mapping same EAs to different RAs\n");
	mmu_unmap((ea_t) ea2, PAGE_16M);
	mmu_map((ea_t) ea2, ptr_2_ra(p2), PP_RWXX, PAGE_16M);
	printk("mapped %p to %p as 16M\n", ea2, p2);
	good = memcmp((void *) ea, (void *) ea2, PAGE_SIZE * 2) != 0;
	printk("mapped %p to %p %scorrectly\n", ea2,
	       p2, !good ? "in" : "");

out:
	printk("finishing up\n");
	mmu_unmap((ea_t) ea2, PAGE_16M);
	mmu_unmap((ea_t) ea, PAGE_16M);

	if (!en) {
		mmu_disable();
	}
}


static void
test_mmu(void)
{
	/*
	 * 1TB - 4K.
	 */
	int res;
	ra_t ra;
	ea_t ea = TB(1) - PAGE_SIZE;
	int en = mmu_enabled();

	if (!en) {
		mmu_enable();
	}

	ra = ptr_2_ra(&_start);
	mmu_map(ea, ra, PP_RWXX, PAGE_4K);
	res = memcmp((void *) ea, (void *) ra, PAGE_SIZE);
	printk("mapped 0x%x to 0x%x %scorrectly\n", ea, ra,
	       res ? "in" : "");
	mmu_unmap(ea, PAGE_4K);
	ra = ptr_2_ra(&_start) + PAGE_SIZE;
	mmu_map(ea, ra, PP_RWXX, PAGE_4K);
	res = memcmp((void *) ea, (void *) ra, PAGE_SIZE);
	printk("mapped 0x%x to 0x%x %scorrectly\n", ea, ra,
	       res ? "in" : "");
	mmu_unmap(ea, PAGE_4K);

	if (!en) {
		mmu_disable();
	}
}


void
menu(void *fdt)
{
	int c = 0;

	/* Clear any chars. */
	while(getchar() != NO_CHAR);

	printk("\nPick your poison:\n");
	do {
		if (c != NO_CHAR) {
			printk("Choices: (MMU = %s):\n"
			       "   (d) 5s delay\n"
			       "   (e) test exception\n"
			       "   (n) test nested exception\n"
			       "   (f) dump FDT\n"
			       "   (M) enable MMU\n"
			       "   (m) disable MMU\n"
			       "   (t) test MMU\n"
			       "   (T) test MMU 16mb pages\n"
			       "   (u) test non-priviledged code\n"
			       "   (I) enable ints\n"
			       "   (i) disable ints\n"
			       "   (H) enable HV dec\n"
			       "   (h) disable HV dec\n"
			       "   (q) poweroff\n",
			       mmu_enabled() ? "enabled" : "disabled");
		}

		c = getchar();
		switch (c) {
		case 'M':
			mmu_enable();
			break;
		case 'm':
			mmu_disable();
			break;
		case 't':
			test_mmu();
			break;
		case 'T':
			test_mmu_16mb();
			break;
		case 'u':
			test_u();
			break;
		case 'f':
			dump_nodes(fdt);
			break;
		case 'q':
			return;
		case 'e':
			printk("Testing exception handling...\n");
			printk("sc(feed) => 0x%x\n", test_syscall(0xfeed,
								  0xface));
			break;
		case 'n':
			printk("Testing nested exception handling...\n");
			printk("sc(dead) => 0x%x\n", test_syscall(0xdead, 0));
			break;
		case 'd':
			time_delay(secs_to_tb(5));
			break;
		case 'I':
			exc_enable_ee();
			break;
		case 'i':
			exc_disable_ee();
			break;
		case 'H':
			exc_enable_hdec();
			break;
		case 'h':
			exc_disable_hdec();
			break;
		}
	} while(1);
}


void
c_main(ra_t fdt_ra)
{
	void *fdt;
	uint64_t len = cpu_to_be64(sizeof(HELLO_OPAL));

#ifndef CONFIG_NOSIM
	/*
	 * Write using sim interface (simpler).
	 */
	mambo_write(HELLO_MAMBO, sizeof(HELLO_MAMBO));
#endif

	/*
	 * Write using firmware interface.
	 */
	opal_write(OPAL_TERMINAL_0, ptr_2_ra(&len), ptr_2_ra(HELLO_OPAL));

	/*
	 * Some info.
	 */
	printk("_start = %p\n", &_start);
	printk("_bss   = %p\n", &_bss_start);
	printk("_stack = %p\n", &_stack_start);
	printk("_end   = %p\n", &_end);
	printk("KPCR   = %p\n", kpcr_get());
	printk("TOC    = %p\n", kpcr_get()->toc);
	printk("OPAL   = %p\n", kpcr_get()->opal_base);
	printk("FDT    = 0x%x\n", fdt_ra);
	fdt = ra_2_ptr(fdt_ra);

	cpu_init(fdt);
	menu(fdt);
}
