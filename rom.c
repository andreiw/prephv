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
#include <layout.h>
#include <mem.h>
#include <ranges.h>
#include <fat_filelib.h>

#define NODE_MUNGE(x) (x + 0x10000000)
#define NODE_DEMUNGE(x) (x - 0x10000000)

#undef WARN
#undef LOG
#define LOG(x, ...)
#define WARN(x, ...)

static FL_FILE *cur_file = NULL;
static void *guest_disk;
static int guest_disk_offset;
static uint64_t guest_disk_len;
static ranges_t guest_mem_avail_ranges;
static ranges_t guest_mem_reg_ranges;
static int memory_node;

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
			} else if (*s == 0xba) {
				con_putchar('|');
			} else if (*s == 0xbb ||
				   *s == 0xc8) {
				con_putchar('\\');
			} else if (*s == 0xbc ||
				   *s == 0xc9) {
				con_putchar('/');
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

	/*
	 * Internally we keep track of all allocations at page granularity,
	 * due to some buggy handling of non-page aligned ranges in veneer.
	 */
	range_remove(&guest_mem_avail_ranges,
		     ALIGN(out, PAGE_SIZE),
		     ALIGN_UP(out + size, PAGE_SIZE) - 1);
	return out;
}


static int32_t
rom_memory_getprop(char *name,
	    uint32_t *dest_len,
	    void *dest_data,
	    uint32_t max_data)
{
	int len;
	const void *data;

	WARN("enter with name %s", name);
	if (!strcmp(name, "reg")) {
		uint32_t *d = dest_data;
		len = range_count(&guest_mem_reg_ranges) * sizeof(uint32_t) * 2;

		if (max_data == 0) {
			*dest_len = len;
		} else {
			*dest_len = min((uint32_t) len, max_data);
		}

		if (d != NULL) {
			range_t *r;

			list_for_each_entry(r, &guest_mem_reg_ranges, link) {
				if (len <= 0) {
					break;
				}

				*d = cpu_to_fdt32((uint32_t) r->base);
				WARN("writing base 0x%x to %p (0x%x)", r->base, d, *d);
				len -= sizeof(uint32_t); d++;

				if (len <= 0) {
					break;
				}

				*d = cpu_to_fdt32((uint32_t) (r->limit - r->base + 1));
				WARN("writing size 0x%x to %p (0x%x)",
				     r->limit - r->base + 1, d, *d);
				len -= sizeof(uint32_t); d++;
			}
		}
	} else if (!strcmp(name, "available")) {
		uint32_t *d = dest_data;
		len = range_count(&guest_mem_avail_ranges) * sizeof(uint32_t) * 2;

		if (max_data == 0) {
			*dest_len = len;
		} else {
			*dest_len = min((uint32_t) len, max_data);
		}

		if (d != NULL) {
			range_t *r;

			list_for_each_entry(r, &guest_mem_avail_ranges, link) {
				/*
				 * Somehow veneer really doesn't like non-page aligned
				 * quantities in the "available" property.
				 */
				uint32_t b = r->base;
				uint32_t bs = r->limit - r->base + 1;

				BUG_ON(b % PAGE_SIZE != 0, "unexpected avail range 0x%x-0x%x",
                                       r->base, r->limit);
				BUG_ON(bs % PAGE_SIZE != 0, "unexpected avail range 0x%x-0x%x",
				       r->base, r->limit);

				if (len <= 0) {
					break;
				}

				*d = cpu_to_fdt32(b);
				len -= sizeof(uint32_t); d++;

				if (len <= 0) {
					break;
				}

				*d = cpu_to_fdt32(bs);
				len -= sizeof(uint32_t); d++;
			}
		}
	} else {
		data = fdt_getprop(prep_dtb,
				   memory_node, name,
				   &len);

		if (data == NULL) {
			return -1;
		}

		WARN("found with len %x", len);
		if (max_data == 0) {
			*dest_len = len;
		} else {
			*dest_len = min((uint32_t) len, max_data);;
		}

		if (dest_data != NULL) {
			memcpy(dest_data, data,
			       min((uint32_t) len, max_data));
		}
	}

	return 0;
}


static int32_t
rom_getprop(int node,
	    char *name,
	    uint32_t *dest_len,
	    void *dest_data,
	    uint32_t max_data)
{
	const void *data;
	int len;

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
	} else if (node == memory_node) {
		return rom_memory_getprop(name,
					  dest_len,
					  dest_data,
					  max_data);
	} else {
		data = fdt_getprop(prep_dtb,
				   node, name,
				   &len);
	}

	if (data == NULL) {
		return -1;
	}

	WARN("found with len %x", len);
	*dest_len = len;

	if (dest_data != NULL) {
		memcpy(dest_data, data, min((uint32_t) len, max_data));
	}

	return 0;
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
			frame->r3 = 0;
			*ihandle = -1;
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

		if (ih == 0xffffffff) {
			BUG_ON(cur_file == NULL, "file not open");
			*outlen = fl_fread(data, len, 1, cur_file);
		} else if (ih == 0xdddddddd) {
			if (guest_disk_offset + len <= guest_disk_len) {
				memcpy(data, guest_disk + guest_disk_offset,
				       len);
				guest_disk_offset += len;
				*outlen = len;
			}
		} else {
			*outlen = rom_stdin(data, len);
		}
		frame->r3 = 0;
	} else if (!strcmp("getproplen", (char *) (uintptr_t) *cia)) {
		int node = *(cia + 3);
		char *name = (char *) (uintptr_t) *(cia + 4);
		uint32_t *lenp = cia + 5;

		WARN("getproplen %s %x", name, node);
		node = NODE_DEMUNGE(node);

		frame->r3 = rom_getprop(node, name, lenp, NULL, 0);
	} else if (!strcmp("getprop", (char *) (uintptr_t) *cia)) {
		int node  = *(cia + 3);
		char *name = (char *) (uintptr_t) *(cia + 4);
		uint32_t *datap = (void *) (uintptr_t) *(cia + 5);
		uint32_t datalen = *(cia + 6);
		uint32_t *lenp = cia + 7;
		node = NODE_DEMUNGE(node);
		
		WARN("getprop node %x %s -> %x (%x bytes) (ptr for size %x)",
		     node, name,
		     (uintptr_t) *(cia + 5),
		     datalen,
		     (uintptr_t) (cia + 7));

		frame->r3 = rom_getprop(node, name, lenp, datap, datalen);
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

		frame->r3 = 0;
		if (strstr(path, "/fake-storage/disk:1,") != NULL) {
			char *fpath = path + strlen("/fake-storage/disk:1");
			char *npath = mem_malloc(strlen(fpath) + 1);
			strcpy(npath, fpath);
			*npath = '/';

			BUG_ON(cur_file != NULL, "1 open allowed at a time");
			cur_file = fl_fopen(npath, "r");
			mem_free(npath);

			if (cur_file == NULL) {
				frame->r3 = -1;
			} else {
				*ih = 0xffffffff;
			}
		} else if (!strcmp(path, "/fake-storage/disk:1")) {
			*ih = 0xdddddddd;
			guest_disk_offset = 0;
		} else {
			ERROR("unknown open path");
			frame->r3 = -1;
			*ih = 0;
		}
	} else if (!strcmp("close", (char *) (uintptr_t) *cia)) {
		uint32_t ih = *(cia + 3);

		if (ih == 0xffffffff) {
			BUG_ON(cur_file == NULL, "file not open");
			fl_fclose(cur_file);
			cur_file = NULL;
			frame->r3 = 0;
		} else if (ih == 0xdddddddd) {
			guest_disk_offset = 0;
			frame->r3 = 0;
		} else {
			frame->r3 = -1;
		}
	} else if (!strcmp("seek", (char *) (uintptr_t) *cia)) {
		uint32_t ih = *(cia + 3);
		uint32_t hi = *(cia + 4);
		uint32_t lo = *(cia + 5);
		length_t offset = ((uint64_t) hi) << 32 | lo;
		if (ih == 0xffffffff) {
			BUG_ON(cur_file == NULL, "file not open");
			if (fl_fseek(cur_file, offset, SEEK_SET) != 0) {
				frame->r3 = -1;
			} else {
				frame->r3 = 0;
			}
		} else if (ih == 0xdddddddd) {
			WARN("seek to 0x%lx", offset);
			guest_disk_offset = offset;
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


static err_t
rom_disk_init(void *fdt)
{
	int node;
	ra_t initrd_start = 0;
	ra_t initrd_end = 0;
	const uint32_t *be32_data;

	node = fdt_path_offset(fdt, "/chosen");
	if (node < 0) {
		ERROR("/chosen not found?");
		return ERR_NOT_FOUND;
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

	if ((initrd_end - initrd_start) == 0) {
		ERROR("missing VM disk (no initrd)");
		return ERR_NOT_FOUND;
	}

	LOG("VM disk @ 0x%lx-0x%lx",
	    initrd_start,
	    initrd_end);

	guest_disk = ra_2_ptr(initrd_start);
	guest_disk_len = initrd_end - initrd_start;

	return ERR_NONE;
}


static int
guest_disk_read(uint32_t sector,
		uint8_t *buffer,
		uint32_t sector_count)
{
	uint32 i;

	for (i = 0;i < sector_count; i++) {
		memcpy(buffer, guest_disk + sector * FAT_SECTOR_SIZE,
		       sector_count * FAT_SECTOR_SIZE);
		sector ++;
		buffer += FAT_SECTOR_SIZE;
	}

	return 1;
}


err_t
rom_init(void *fdt)
{
	err_t err;
	FL_FILE *fl;
	length_t veneer_len;
	uint32_t hvcall = 0x44000022; /* sc 1 */

	BUG_ON(guest == NULL, "guest state invalid");
	BUG_ON(guest->ram_size <= MB(16), "guest RAM too small");

	range_init(&guest_mem_avail_ranges);
	range_init(&guest_mem_reg_ranges);
	range_add(&guest_mem_avail_ranges, 0, guest->ram_size - 1);
	range_add(&guest_mem_reg_ranges, 0, guest->ram_size - 1);

	guest->rom.claim_arena_start = guest->ram_size - MB(16);
	guest->rom.claim_arena_ptr = guest->rom.claim_arena_start;
	guest->rom.claim_arena_end = guest->ram_size;

	/*
	 * We'll special case property accesses for RAM since
	 * we need to feed live available ranges.
	 */
	memory_node = fdt_node_offset_by_dtype(prep_dtb, -1, "memory");
	BUG_ON(memory_node == -1, "memory node missing from DT template");

	err = rom_disk_init(fdt);
	if (err != ERR_NONE) {
		return err;
	}

	fl_init();
	if (fl_attach_media(guest_disk_read, NULL) != FAT_INIT_OK) {
		return ERR_UNSUPPORTED;
	}

	fl = fl_fopen("/veneer.exe", "r");
	if (fl == NULL) {
		ERROR("No veneer.exe on VM disk");
		return ERR_NOT_FOUND;
	}

	if (fl_fseek(fl, 0, SEEK_END) != 0) {
		ERROR("Seek (end) failure");
		return ERR_UNSUPPORTED;
	}

	veneer_len = fl_ftell(fl);
	LOG("ARC veneer is 0x%lx bytes", veneer_len);

	if (fl_fseek(fl, 0x200, SEEK_SET) != 0) {
		ERROR("Seek (0x200) failure");
		return ERR_UNSUPPORTED;
	}
	veneer_len -= 0x200;

	rom_claim(0x50000, veneer_len, 0);
	if (fl_fread((void *) (LAYOUT_VM_START + 0x00050000),
		     veneer_len, 1, fl) != veneer_len) {
		ERROR("Read failure");
		return ERR_UNSUPPORTED;
	}

	fl_fclose(fl);
	lwsync();
	flush_cache(LAYOUT_VM_START + 0x00050000, veneer_len);

	rom_claim(4, 4, 0);
	memcpy((void *) (LAYOUT_VM_START + 0x4), &hvcall, 4);
	lwsync();
	flush_cache(LAYOUT_VM_START + 0x4, 4);

	return ERR_NONE;
}
