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
#include <log.h>

#define HELLO_MAMBO "Hello Mambo!\n"
#define HELLO_OPAL "Hello OPAL!\n"

kpcr_t kpcr;
static time_req_t opal_timer;

static void
dump_props(void *fdt, int node, int depth)
{
	const char *n;
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

			if (fdt32_to_cpu(prop->len) == 0) {
				LOG("%.*c%s: true", depth, ' ', n);
			} else if (!strcmp(n, "compatible") ||
				   !strcmp(n, "bootargs") ||
				   !strcmp(n, "linux,stdout-path") ||
				   !strcmp(n, "epapr-version") ||
				   !strcmp(n, "model") ||
				   !strcmp(n, "device_type")
				) {
				LOG("%.*c%s: %s", depth, ' ', n, prop->data);
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
				   !strcmp(n, "linux,initrd-start") ||
				   !strcmp(n, "linux,initrd-end") ||
				   !strcmp(n, "phandle")) {
				LOG("%.*c%s: 0x%x", depth, ' ', n,
				       be32_to_cpu(*(uint32_t *) prop->data));
			} else {
				LOG("%.*c%s: 0x%x@0x%x", depth, ' ', n,
				       fdt32_to_cpu(prop->len),
				       prop->data);
			}
		}

		offset = nextoffset;
	} while ((tag != FDT_BEGIN_NODE) && (tag != FDT_END_NODE));
}


static bool_t
opal_timer_cb(time_req_t *t) {
	opal_poll_events(0);

	/* Recurring timer. */
	t->when = ms_to_tb((uint64_t) t->ctx) + mftb();
	return true;
}


static void
cpu_init(void *fdt)
{
	int node;
	unsigned int slb_size;
	unsigned int tb_freq;
	const uint32_t *be32_data;
	uint64_t opal_ms = 0;

	node = fdt_node_offset_by_dtype(fdt, -1, "cpu");
	if (node < 0) {
		LOG("CPU0 not found?");
		return;
	}

	be32_data = fdt_getprop(fdt, node, "slb-size", NULL);
	if (be32_data != NULL) {
		slb_size = be32_to_cpu(*be32_data);
	} else {
		LOG("Assuming default SLB size");
		slb_size = 32;
	}
	LOG("SLB size = 0x%x", slb_size);
	kpcr_get()->slb_size = slb_size;

	be32_data = fdt_getprop(fdt, node, "timebase-frequency", NULL);
	if (be32_data != NULL) {
		tb_freq = be32_to_cpu(*be32_data);
	} else {
		LOG("Assuming default TB frequency");
		tb_freq = 512000000;
	}
	LOG("TB freq = %u", tb_freq);
	kpcr_get()->tb_freq = tb_freq;

	/*
	 * We might need to call back into OPAL periodically.
	 * On real HW this (might?) prevent a watchdog-related reboot.
	 */
	node = fdt_node_check_compatible(fdt, -1, "ibm,opal-v2");
	if (node >= 0) {
		be32_data = fdt_getprop(fdt, node, "ibm,heartbeat-ms", NULL);
		if (be32_data != NULL) {
			opal_ms = be32_to_cpu(*be32_data);
		}
	}

	exc_init();
	mmu_init(4UL * 1024 * 1024 * 1024);
	time_init();

	if (opal_ms != 0) {
		/*
		 * We might need to call back into OPAL periodically.
		 * On real HW this (might?) prevent a watchdog-related reboot.
		 */
		time_prep_ms(opal_ms, opal_timer_cb, "opal",
			     (void *) opal_ms, &opal_timer);
		time_enqueue(&opal_timer);
	}

	/*
	 * Enable interrupts.
	 */
	exc_enable_ee();
	LOG("we are alive!");

	LOG("We're alive!");
	WARN("We're alive!");
	ERROR("We're alive!");

	{
		void dump_nodes(void *fdt);
		dump_nodes(fdt);
	}

	FATAL("We're alive!");
	while(1);
}


 void
dump_nodes(void *fdt)
{
	int i;
	int numrsv;
	int offset;
	int depth;
	const char *n;

	if (fdt_check_header(fdt) != 0) {
		LOG("Bad FDT");
		return;
	}

	numrsv = fdt_num_mem_rsv(fdt);
	LOG("FDT version: %d", fdt_version(fdt));

	for (i = 0; i < numrsv; i++) {
		ra_t addr;
		length_t size;
		if (fdt_get_mem_rsv(fdt, i, &addr, &size) != 0) {
			break;
		}

		LOG("/memreserve/ 0x%x 0x%x;",
		       addr, size);
	}

	offset = 0;
	depth = 0;
	do {
		n = fdt_get_name(fdt, offset, NULL);
		if (n != NULL) {
			LOG("%.*c<%s>", depth * 4, ' ', n);
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
	asm volatile("sc 1" : "=r" (r3) : "r" (r3), "r" (r4));
	return r3;
}


static void
test_hv(void)
{
	static void *upage = NULL;
	eframe_t uframe;

	if (upage == NULL) {
		upage = mem_alloc(PAGE_SIZE, PAGE_SIZE);
	}

	/*
	 * Grab a page and map it as "virtual real mode" page 0 in our VM.
	 */
	mmu_map_vrma(ptr_2_ra(upage), ptr_2_ra(upage) + PAGE_SIZE);
	memcpy(upage, (void *) test_syscall, (uint64_t) &test_hv -
	       (uint64_t) &test_syscall);
	lwsync();
	flush_cache((vaddr_t) upage, PAGE_SIZE);

	/*
	 * VM supervisor is at EA 0 and does test_syscall(0x1337, 0),
	 * which simply returns to HV supervisor state.
	 */
	uframe.r1 = 0 + PAGE_SIZE - STACKFRAMESIZE;
	uframe.r3 = 0x1337;
	uframe.r4 = 0;
	uframe.hsrr0 = 0;
	/* No interrupts, no MMU. */
	uframe.hsrr1 = (mfmsr() & ~(MSR_HV | MSR_IR | MSR_DR | MSR_EE));

	/*
	 * Force switch into our "VM". The exception handler stashes
	 * the kernel state behind in a global (sigh), which is magically
	 * restored on a test_syscall(0x1337, 0). The things we do
	 * to avoid writing an actual scheduler.
	 */
	test_syscall(0x7e57, (uint64_t) &uframe);

	/*
	 * We return here. Unmap VM "real mode" memory.
	 */
	mmu_unmap_vrma(ptr_2_ra(upage), ptr_2_ra(upage) + PAGE_SIZE);
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
	memcpy((void *) ea, (void *) test_syscall, (uint64_t) &test_hv -
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

	/*
	 * Because we create the mapping in the identity segment, which has
	 * base page size = 4K, 16M pages are achieved using MPSS. This is
	 * better than not using MPSS and manually populating 4K entries,
	 * becase at the TLB and ERAT level there should be only one entry.
	 *
	 * On the sim, of course, MPSS is still worse than using a segment
	 * where the base page size = 16M, because we need to do more
	 * oh so glacially slow PTE updates.
	 */
	LOG("mapping two EAs to same RA - on a sim this will take a while...");
	mmu_map((ea_t) ea, ptr_2_ra(p1), PP_RWXX, PAGE_16M);
	LOG("mapped 0x%x to 0x%x as 16M", ea, p1);
	mmu_map((ea_t) ea2, ptr_2_ra(p1), PP_RWXX, PAGE_16M);
	LOG("mapped 0x%x to 0x%x as 16M", ea2, p1);
	for (i = 0; i < (PAGE_SIZE * 2 / sizeof(uint64_t)); i++) {
		ea[i] = (uint64_t) &p1[i];
	}
	LOG("prep completed");
	good = true;
	for (i = 0; i < (PAGE_SIZE * 2 / sizeof(uint64_t)); i++) {
		if (ea2[i] != (uint64_t) &p1[i]) {
			good = false;
			LOG("error at 0x%x: expected 0x%x got 0x%x",
			       &ea2[i], (uint64_t) &p1[i], ea2[i]);
			break;
		}
	}
	LOG("16M MPSS mappings %swork", !good ? "don't " : "");
	if (!good) {
		goto out;
	}

	LOG("mapping same EAs to different RAs");
	mmu_unmap((ea_t) ea2, PAGE_16M);
	mmu_map((ea_t) ea2, ptr_2_ra(p2), PP_RWXX, PAGE_16M);
	LOG("mapped %p to %p as 16M", ea2, p2);
	good = memcmp((void *) ea, (void *) ea2, PAGE_SIZE * 2) != 0;
	LOG("mapped %p to %p %scorrectly", ea2,
	       p2, !good ? "in" : "");

out:
	LOG("finishing up");
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
	void *source;
	ea_t ea = TB(1) - PAGE_SIZE;
	int en = mmu_enabled();

	if (!en) {
		mmu_enable();
	}

	source = (void *) &_start;
	mmu_map(ea, ptr_2_ra(source), PP_RWXX, PAGE_4K);
	res = memcmp((void *) ea, source, PAGE_SIZE);
	LOG("mapped 0x%x to 0x%x %scorrectly", ea,
	       ptr_2_ra(source), res ? "in" : "");
	mmu_unmap(ea, PAGE_4K);

	source = (void *) (((ea_t) &_start) + PAGE_SIZE);
	mmu_map(ea, ptr_2_ra(source), PP_RWXX, PAGE_4K);
	res = memcmp((void *) ea, source , PAGE_SIZE);
	LOG("mapped 0x%x to 0x%x %scorrectly", ea,
	       ptr_2_ra(source), res ? "in" : "");
	mmu_unmap(ea, PAGE_4K);

	if (!en) {
		mmu_disable();
	}
}


static bool_t
timer_cb(time_req_t *t)
{
	LOG("Timer '%s' fired!", t->name);

	/* This is a recurring timer. */
	t->when = secs_to_tb((uint64_t) t->ctx) + mftb();
	return true;
}


static void
toggle_timer(void)
{
	static time_req_t t;
	static bool_t on = false;

	if (!on) {
		time_prep_s(5, timer_cb, "5s", (void *) 5, &t);
		LOG("Enabling timer callback");
		time_enqueue(&t);
	} else {
		LOG("Disabling timer callback");
		time_dequeue(&t);
	}

	on ^= true;
}


static void
run_initrd(void *fdt)
{
	int node;
	ra_t initrd_start;
	ra_t initrd_end;
	const uint32_t *be32_data;
	void (*run)(ra_t, ra_t, ra_t);
	ra_t ra;

	node = fdt_path_offset(fdt, "/chosen");
	if (node < 0) {
		LOG("/chosen not found?");
		return;
	}

	be32_data = fdt_getprop(fdt, node,
				"linux,initrd-start",
				NULL);
	if (be32_data != NULL) {
		initrd_start = be32_to_cpu(*be32_data);
	}

	be32_data = fdt_getprop(fdt, node,
				"linux,initrd-end",
				NULL);
	if (be32_data != NULL) {
		initrd_end = be32_to_cpu(*be32_data);
	}

	LOG("initrd is 0x%x-0x%x",
	       initrd_start,
	       initrd_end);
	if ((initrd_end - initrd_start) == 0) {
		LOG("nothing to do, empty initrd");
		return;
	}

	ra = *(ra_t *) ra_2_ptr(initrd_start);
	if (ra >= initrd_start &&
	    ra < initrd_end) {
		/*
		 * Could be a pointer to code (ABIv2)
		 * or to a function descriptor (ABIv1)
		 */
		ra = *(ra_t *) ra_2_ptr(ra);
		if (ra >= initrd_start &&
		    ra < initrd_end) {
			/*
			 * ABIv1 function descriptor.
			 */
			LOG("ABIv1");
			run = (void *) ra;
		} else {
			/*
			 * ABIv2.
			 */
			LOG("ABIv2");
			run = (void *) * (ra_t *)
				ra_2_ptr(initrd_start);
		}
	} else {
		/*
		 * Straight code?
		 */
		LOG("looks like straight binary");
		run = (void *) initrd_start;
	}

	LOG("calling 0x%x", run);
	exc_disable_ee();
	mmu_disable();
	run(ptr_2_ra(fdt), kpcr_get()->opal_base, kpcr_get()->opal_entry);
	LOG("returned?");
}


void
menu(void *fdt)
{
	int c = 0;

	/* Clear any chars. */
	while(con_getchar() != NO_CHAR);

	LOG("\nPick your poison:");
	do {
		if (c != NO_CHAR) {
			LOG("Choices: (MMU = %s):\n"
			       "   (d) 5s delay\n"
			       "   (D) toggle 5s timer\n"
			       "   (e) test exception\n"
			       "   (n) test nested exception\n"
			       "   (f) dump FDT\n"
			       "   (M) enable MMU\n"
			       "   (m) disable MMU\n"
			       "   (t) test MMU\n"
			       "   (T) test MMU 16mb pages\n"
			       "   (u) test non-priviledged code\n"
			       "   (U) test VM real-mode code\n"
			       "   (H) enable HV dec\n"
			       "   (h) disable HV dec\n"
			       "   (I) run initrd",
			       mmu_enabled() ? "enabled" : "disabled");
		}

		c = con_getchar();
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
		case 'U':
			test_hv();
			break;
		case 'f':
			dump_nodes(fdt);
			break;
		case 'e':
			LOG("Testing exception handling...");
			LOG("sc(feed) => 0x%x", test_syscall(0xfeed,
								  0xface));
			break;
		case 'n':
			LOG("Testing nested exception handling...");
			LOG("sc(dead) => 0x%x", test_syscall(0xdead, 0));
			break;
		case 'd':
			time_delay(secs_to_tb(5));
			break;
		case 'D':
			toggle_timer();
			break;
		case 'H':
			set_HDEC(DEC_DISABLE);
			exc_enable_hdec();
			break;
		case 'h':
			exc_disable_hdec();
			break;
		case 'I':
			run_initrd(fdt);
			break;
		}
	} while(1);
}

void
c_main(ra_t fdt_ra)
{
	void *fdt;
	uint64_t len = cpu_to_be64(sizeof(HELLO_OPAL));

#ifdef CONFIG_MAMBO
	/*
	 * Write using sim interface (simpler).
	 */
	mambo_write(HELLO_MAMBO, sizeof(HELLO_MAMBO));
#endif /* CONFIG_MAMBO */

	/*
	 * Write using firmware interface.
	 */
	opal_write(OPAL_TERMINAL_0, ptr_2_ra(&len), ptr_2_ra(HELLO_OPAL));

	/*
	 * Some info.
	 */
	LOG("_start = %p", &_start);
	LOG("_bss   = %p", &_bss_start);
	LOG("_stack = %p", &_stack_start);
	LOG("_end   = %p", &_end);
	LOG("KPCR   = %p", kpcr_get());
	LOG("TOC    = %p", kpcr_get()->toc);
	LOG("OPAL   = %p", kpcr_get()->opal_base);
	LOG("FDT    = 0x%x", fdt_ra);
	fdt = ra_2_ptr(fdt_ra);

	cpu_init(fdt);
}
