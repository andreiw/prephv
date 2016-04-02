/*
 * OF emulation.
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

#include <rom.h>
#include <string.h>
#include <console.h>
#include <guest.h>
#include <mmu.h>
#include <prep_dtb.h>
#include <libfdt.h>
#include <time.h>

#undef WARN
#undef LOG
#undef ERROR
#define LOG(x, ...)
#define WARN(x, ...)
#define ERROR(X, ...)

uint32_t
rom_claim(uint32_t addr, uint32_t size, uint32_t align)
{
	uint32_t out;

	if (align != 0) {
		guest->rom.claim_arena_ptr =
			ALIGN_UP(guest->rom.claim_arena_ptr,
				 align);

		BUG_ON(guest->rom.claim_arena_ptr >=
		       guest->rom.claim_arena_end, "claim arena overflow");

		out = (uint32_t) guest->rom.claim_arena_ptr;
		guest->rom.claim_arena_ptr += size;

		BUG_ON(guest->rom.claim_arena_ptr >
		       guest->rom.claim_arena_end, "claim arena overflow");
	} else {
		out = addr;
	}

	return out;
}

err_t
rom_call(eframe_t *frame)
{
	uint32_t *cia = (uint32_t*) frame->r3;
	LOG("OF call %s from 0x%lx in %x out %x",
	    (char *) (uintptr_t) *cia,
	    frame->lr,
	    (char *) (uintptr_t) *(cia + 1),
	    (char *) (uintptr_t) *(cia + 2));

	if (!strcmp("finddevice", (char *) (uintptr_t) *cia)) {
		int node;
		
		char *dev = (char *) (uintptr_t) *(cia + 3);
		int *ihandle = (int *) (uintptr_t) (cia + 4);

		node = fdt_path_offset(prep_dtb, dev);
		if (node < 0) {
			WARN("%s -> not found", dev);
			frame->r3 = -1;
		} else {
			WARN("%s -> %x @ %x", dev, node, ihandle);
			*ihandle = node;
			frame->r3 = 0;
		}
	} else if (!strcmp("milliseconds", (char *) (uintptr_t) *cia)) {
		uint32_t *ms = (cia + 3);
		*ms = tb_to_ms(mftb());
		frame->r3 = 0;
	} else if (!strcmp("peer",(char *) (uintptr_t) *cia)) {
		ERROR("%x %x %x", *(cia + 1), *(cia + 2), *(cia + 3));
		int node = *(cia + 3);
		int n;

		if (node == 0) {
			n = fdt_subnode(prep_dtb, node);
		} else {
			n = fdt_sibling(prep_dtb, node);
		}

		if (n < 0) {
			ERROR("%x has no peer", node);
			frame->r3 = -1;
		} else {
			ERROR("%x has peer %x", node, n);
			*(cia + 4) = n;
			frame->r3 = 0;
		}
	} else if (!strcmp("child",(char *) (uintptr_t) *cia)) {
		int node = *(cia + 3);

		if (node == 0) {
			frame->r3 = -1;
		} else {
			int n = fdt_subnode(prep_dtb, node);
			if (n < 0) {
				ERROR("%x has no child", node);
				frame->r3 = -1;
			} else {
				ERROR("%x has child %x", node, n);
				*(cia + 4) = n;
				frame->r3 = 0;
			}
		}
	} else if (!strcmp("parent",(char *) (uintptr_t) *cia)) {
		int node = *(cia + 3);

		if (node == 0) {
			frame->r3 = -1;
		} else {
			int n = fdt_parent_offset(prep_dtb, node);
			if (n < 0) {
				ERROR("%x has no parent", node);
				frame->r3 = -1;
			} else {
				ERROR("%x has parent %x", node, n);
				*(cia + 4) = n;
				frame->r3 = 0;
			}
		}
	} else if (!strcmp("exit", (char *) (uintptr_t) *cia)) {
		while(1);
	} else if (!strcmp("write", (char *) (uintptr_t) *cia)) {
		con_puts_len((char *) ra_2_ptr((uintptr_t) *(cia + 4)),
			     (uintptr_t) *(cia + 5));
	} else if (!strcmp("getproplen", (char *) (uintptr_t) *cia)) {
		const void *data;
		int node  = *(cia + 3);
		char *name = (char *) (uintptr_t) *(cia + 4);
		uint32_t *lenp = cia + 5;
		int len;

		WARN("getproplen %s", name);

		if (!strcmp(name, "name")) {
			data = fdt_get_name(prep_dtb,
					    node,
					    &len);
			BUG_ON(data == NULL, "fdt_get_name failed");
		} else {
			data = fdt_getprop(prep_dtb,
					   node, name,
					   &len);
		}

		if (data == NULL) {
			frame->r3 = -1;
		} else {
			WARN("found with len %x", len);
			*lenp = len;
			frame->r3 = 0;
		}
	} else if (!strcmp("getprop", (char *) (uintptr_t) *cia)) {
		const void *data;
		int node  = *(cia + 3);
		char *name = (char *) (uintptr_t) *(cia + 4);
		int len;
		
		WARN("node %x %s -> %x (%x bytes) (ptr for size %x)",
		     node, name,
		     (uintptr_t) *(cia + 5),
		     (uintptr_t) *(cia + 6),
		     (uintptr_t) (cia + 7));

		if (!strcmp(name, "name")) {
			data = fdt_get_name(prep_dtb,
					    node,
					    &len);
			BUG_ON(data == NULL, "fdt_get_name failed");
		} else {
			data = fdt_getprop(prep_dtb,
					   node,
					   name,
					   &len);
		}

		if (data == NULL) {
			frame->r3 = -1;
		} else {
			WARN("found with len %x", len);
			memcpy((void *) (uintptr_t) *(cia + 5),
			       data, len);
			*(uint32_t *) (uintptr_t) (cia + 7) = len;
			frame->r3 = 0;
		}
	} else if (!strcmp("call-method", (char *) (uintptr_t) *cia)) {
		WARN("in %x out %x %s on ihandle %x",
		     (char *) (uintptr_t) *(cia + 1),
		     (char *) (uintptr_t) *(cia + 2),
		     (char *) (uintptr_t) *(cia + 3),
		     (char *) (uintptr_t) *(cia + 4));

		if (!strcmp("claim", (char *) (uintptr_t) *(cia + 3))) {
			WARN("align 0x%lx", (char *) (uintptr_t) *(cia + 5));
			WARN("size 0x%lx", (uintptr_t) *(cia + 6));
			WARN("addr 0x%lx", (uintptr_t) *(cia + 7));
			WARN("claim result 0x%lx", (uintptr_t) (cia + 8));
			WARN("outer result 0x%lx", (uintptr_t) (cia + 9));
			
			*(cia + 8) = rom_claim(*(cia + 7), *(cia + 6), *(cia + 5));
			*(cia + 9) = 0;
			frame->r3 = 0;
		} else if (!strcmp("map", (char *) (uintptr_t) *(cia + 3))) {
			uint32_t virt =  *(cia + 7);
			uint32_t phys =  *(cia + 8);
			WARN("mode 0x%lx", (uintptr_t) *(cia + 5));
			WARN("size 0x%lx", (uintptr_t) *(cia + 6));
			WARN("virtual 0x%lx",  (uintptr_t) virt);
			WARN("physical 0x%lx", (uintptr_t) phys);

			if (phys == virt) {
				*(cia + 9) = 0; // map succeeded
			} else {
				ERROR("virt != phys mapping unsupported");
				*(cia + 9) = -1; // map failed
			}
			frame->r3 = 0;
		}
	} else if (!strcmp("claim", (char *) (uintptr_t) *cia)) {
		WARN("0x%lx", (char *) (uintptr_t) *(cia + 1));
		WARN("0x%lx", (char *) (uintptr_t) *(cia + 2));
		WARN("addr 0x%lx", (char *) (uintptr_t) *(cia + 3));
		WARN("size 0x%lx", (char *) (uintptr_t) *(cia + 4));
		WARN("align 0x%lx", (char *) (uintptr_t) *(cia + 5));
		WARN("0x%lx", (char *) (uintptr_t) *(cia + 6));

		*(cia + 6) = rom_claim(*(cia + 3), *(cia + 4), *(cia + 5));
		frame->r3 = 0;
	} else {
		ERROR("unknown %s", (char *) (uintptr_t) *cia);
	}

	frame->hsrr0 = frame->lr;
	return ERR_NONE;
}

