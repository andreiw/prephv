/*
 * PReP HV, a huge hack.
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
#include <guest.h>
#include <layout.h>

kpcr_t kpcr;
static time_req_t opal_timer;


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
}



void
c_main(ra_t fdt_ra)
{
#define HELLO_PREPHV "Hello, PReP HV!\n"
	void *fdt;
	uint64_t len = cpu_to_be64(sizeof(HELLO_PREPHV));

	/*
	 * Write using firmware interface.
	 */
	opal_write(OPAL_TERMINAL_0, ptr_2_ra(&len), ptr_2_ra(HELLO_PREPHV));

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

	/*
	 * Guest memory.
	 */
	{
		err_t err;
		eframe_t uframe;

		err = guest_init(MB(64));
		BUG_ON(err != ERR_NONE, "guest init failed");

		err = rom_init(fdt);
		BUG_ON(err != ERR_NONE, "rom init failed");

		memset(&uframe, 0, sizeof(uframe));
		uframe.r1 = LAYOUT_VM_START + 0x00050000 - STACKFRAMESIZE;
		uframe.r5 = LAYOUT_VM_START + 0x4;
		uframe.hsrr0 = LAYOUT_VM_START + 0x00050000;
		uframe.hsrr1 = (mfmsr() ^ MSR_SF) | MSR_PR;

		kpcr_get()->kern_sp = (uint64_t) &uframe;
		exc_disable_ee();
		exc_rfi(&uframe);
	}

	while(1);
}
