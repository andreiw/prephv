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

#include <setupldr.h>

#define NODE_MUNGE(x) (x + 0x10000000)
#define NODE_DEMUNGE(x) (x - 0x10000000)

#undef WARN
#undef LOG
#define LOG(x, ...)
#define WARN(x, ...)

int cur_offset;
int cur_len;
void *cur_base = NULL;

void *guest_disk;
int guest_disk_offset;
uint64_t guest_disk_len;

int
rom_stdin(char *buf, uint32_t len)
{
	char *start = buf;
	char *end = buf + len;

	while (buf < end) {
		static bool_t csi = false;
		char prev_c = 0;
		int c;

		if (prev_c != 0) {
			*buf = prev_c;
			buf++;
			prev_c = 0;
		} else {
			c = con_getchar();
			if (c == NO_CHAR) {
				break;
			}

			if (csi) {
				if (c == '[') {
					*buf = 0x9b;
					buf++;
				} else {
					*buf = '\33';
					buf++;
					prev_c = c;
				}

				csi = false;
			} else {
				if (c == '\33') {
					csi = true;
				} else {
					*buf = c;
					buf++;
				}
			}
		}
	}

	return buf - start;
}

void
rom_stdout(char *s, uint32_t len)
{
	while (len--) {
		if (*s == 0x9b) {
			con_putchar('\33');
			con_putchar('[');
		} else {
			if (*s == 0xcd) {
				con_putchar('=');
			} else {
				con_putchar(*s);
			}
		}
		s++;
	}
}


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
			*ihandle = NODE_MUNGE(node);
			frame->r3 = 0;
		}
	} else if (!strcmp("milliseconds", (char *) (uintptr_t) *cia)) {
		uint32_t *ms = (cia + 3);
		*ms = tb_to_ms(mftb());
		frame->r3 = 0;
	} else if (!strcmp("peer",(char *) (uintptr_t) *cia)) {
		int node = *(cia + 3);
		int n;

		if (node == 0) {
			n = 0;
		} else {
			node = NODE_DEMUNGE(node);
			n = fdt_sibling(prep_dtb, node);
		}

		if (n < 0) {
			frame->r3 = -1;
		} else {
			*(cia + 4) = NODE_MUNGE(n);
			frame->r3 = 0;
		}
	} else if (!strcmp("child",(char *) (uintptr_t) *cia)) {
		int node = *(cia + 3);

		if (node == 0) {
			frame->r3 = -1;
		} else {
			node = NODE_DEMUNGE(node);

			int n = fdt_subnode(prep_dtb, node);
			if (n < 0) {
				frame->r3 = -1;
			} else {
				*(cia + 4) = NODE_MUNGE(n);
				frame->r3 = 0;
			}
		}
	} else if (!strcmp("parent",(char *) (uintptr_t) *cia)) {
		int node = *(cia + 3);

		if (node == 0) {
			frame->r3 = -1;
		} else {
			node = NODE_DEMUNGE(node);

			int n = fdt_parent_offset(prep_dtb, node);
			if (n < 0) {
				frame->r3 = -1;
			} else {
				*(cia + 4) = NODE_MUNGE(n);
				frame->r3 = 0;
			}
		}
	} else if (!strcmp("exit", (char *) (uintptr_t) *cia) ||
		   !strcmp("boot", (char *) (uintptr_t) *cia)) {
		ERROR("\nVM reboot requested - hanging");
		while(1);
	} else if (!strcmp("write", (char *) (uintptr_t) *cia)) {
		char *data = (char *) (uintptr_t) *(cia + 4);
		uint32_t len = *(cia + 5);
		rom_stdout(data, len);
		frame->r3 = 0;
	} else if (!strcmp("read", (char *) (uintptr_t) *cia)) {
		uint32_t ih = *(cia + 3);
		char *data = (char *) (uintptr_t) *(cia + 4);
		uint32_t len = *(cia + 5);
		uint32_t *outlen = (cia + 6);

		if (ih == 0x12345678) {
			WARN("read %x bytes from 0x%lx (end 0x%lx)", len, cur_base + cur_offset, cur_base + cur_len);
			if (cur_offset + len <= cur_len) {
				memcpy(data, cur_base + cur_offset,
				       len);
				cur_offset += len;
				*outlen = len;
			}
		} else {
			*outlen = rom_stdin(data, len);
		}
		frame->r3 = 0;
	} else if (!strcmp("getproplen", (char *) (uintptr_t) *cia)) {
		const void *data;
		int node = *(cia + 3);
		char *name = (char *) (uintptr_t) *(cia + 4);
		uint32_t *lenp = cia + 5;
		int len;

		WARN("getproplen %s %x", name, node);
		node = NODE_DEMUNGE(node);

		if (!strcmp(name, "name")) {
			if (node == 0) {
				data = "/";
				len = 1;
			} else {
				data = fdt_get_name(prep_dtb,
						    node,
						    &len);
				BUG_ON(data == NULL, "fdt_get_name failed for 0x%x", node);
			}
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
		node = NODE_DEMUNGE(node);
		int len;
		
		WARN("node %x %s -> %x (%x bytes) (ptr for size %x)",
		     node, name,
		     (uintptr_t) *(cia + 5),
		     (uintptr_t) *(cia + 6),
		     (uintptr_t) (cia + 7));

		if (!strcmp(name, "name")) {
			if (node == 0) {
				data = "/";
				len = 1;
			} else {
				data = fdt_get_name(prep_dtb,
						    node,
						    &len);
				BUG_ON(data == NULL, "fdt_get_name failed for %x", node);
			}
		} else {
			data = fdt_getprop(prep_dtb,
					   node,
					   name,
					   &len);
		}

		if (data == NULL) {
			frame->r3 = -1;
		} else {
			WARN("found with len %x data %x", len, *(uint32_t *) data);
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
			uint32_t size = *(cia + 6);
			WARN("mode 0x%lx", (uintptr_t) *(cia + 5));
			WARN("size 0x%lx", size);
			WARN("virtual 0x%lx",  (uintptr_t) virt);
			WARN("physical 0x%lx", (uintptr_t) phys);

			if (phys == virt) {
				*(cia + 9) = 0; // map succeeded
			} else {
				WARN("mapping phys 0x%x (RAM 0x%lx RA 0x%lx) to 0x%x",
				     phys, phys + guest->ram,
				     ptr_2_ra(phys + guest->ram), virt);
				mmu_map_range(virt, virt + size,
					      ptr_2_ra(phys + guest->ram),
					      PP_RWRW, PAGE_4K);
				*(cia + 9) = 0; // success
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
	} else if (!strcmp("instance-to-package", (char *) (uintptr_t) *cia)) {
		int node;
		int ih = *(cia + 3);
		uint32_t *ph = (cia + 4);

		node = fdt_node_offset_by_phandle(prep_dtb,
						  ih);
		if (node < 0) {
			frame->r3 = -1;
		} else {
			*ph = NODE_MUNGE(node);
			frame->r3 = 0;
		}
	} else if (!strcmp("instance-to-path", (char *) (uintptr_t) *cia)) {
		int node;
		int i = *(cia + 3);
		char *buf = (char *) (uintptr_t) *(cia + 4);
		int buflen = *(cia + 5);
		uint32_t *outlen = (cia + 6);
		WARN("i2p %x -> buf at %x len %x", i, buf, buflen);

		if (buflen == 0) {
			*outlen = 120;
			frame->r3 = 0;
		} else {
			node = fdt_node_offset_by_phandle(prep_dtb,
							  i);
			if (node < 0) {
				frame->r3 = -1;
			} else {
				if (fdt_get_path(prep_dtb, node, buf, buflen) == 0) {
					WARN("get path got %s", buf);
					frame->r3 = 0;
					*outlen = buflen;
				} else {
					frame->r3 = -1;
				}
			}
		}
	} else if (!strcmp("package-to-path", (char *) (uintptr_t) *cia)) {
		int node;
		int p = *(cia + 3);
		char *buf = (char *) (uintptr_t) *(cia + 4);
		int buflen = *(cia + 5);
		uint32_t *outlen = (cia + 6);

		WARN("p2p %x -> buf at %x len %x", p, buf, buflen);

		if (buflen == 0) {
			*outlen = 120;
			frame->r3 = 0;
		} else {
			node = NODE_DEMUNGE(p);
			if (node < 0) {
				frame->r3 = -1;
			} else {
				if (fdt_get_path(prep_dtb, node, buf, buflen) == 0) {
					WARN("get path got %s", buf);
					frame->r3 = 0;
					*outlen = buflen;
				} else {
					frame->r3 = -1;
				}
			}
		}
	} else if (!strcmp("open", (char *) (uintptr_t) *cia)) {
		uint32_t *ih = (cia + 4);
		char *path = (char *) (uintptr_t) *(cia + 3);
		WARN("open %s", path);

		*ih = 0x12345678;
		frame->r3 = 0;

		BUG_ON(cur_base != NULL, "only 1 open supported at a time");
		if (!strcmp(path, "/fake-storage/disk:1,osloader.exe")) {
			cur_offset = 0;
			cur_base = SETUPLDR;
			cur_len = SETUPLDR_len;
		} else if (!strcmp(path, "/fake-storage/disk:1")) {
			cur_offset = 0;
			cur_base = guest_disk;
			cur_len = guest_disk_len;
		} else {
			ERROR("unknown open path");
			frame->r3 = -1;
			*ih = 0;
		}
	} else if (!strcmp("close", (char *) (uintptr_t) *cia)) {
		uint32_t ih = *(cia + 3);

		if (ih == 0x12345678) {
			cur_offset = 0;
			cur_base = NULL;
			cur_len = 0;
			frame->r3 = 0;
		} else {
			frame->r3 = -1;
		}
	} else if (!strcmp("seek", (char *) (uintptr_t) *cia)) {
		uint32_t ih = *(cia + 3);
		uint32_t hi = *(cia + 4);
		uint32_t lo = *(cia + 5);
		int offset = ((uint64_t) hi) << 32 | lo;
		if (ih == 0x12345678) {
			WARN("seek to 0x%lx", offset);
			cur_offset = offset;
			frame->r3 = 0;
		} else {
			frame->r3 = -1;
		}
	} else {
		ERROR("unknown %s", (char *) (uintptr_t) *cia);
	}

	frame->hsrr0 = frame->lr;
	return ERR_NONE;
}

