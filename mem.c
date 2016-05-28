/*
 * Basic allocator.
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
#include <defs.h>
#include <linkage.h>
#include <assert.h>
#include <libfdt.h>
#include <list.h>
#include <mmu.h>
#include <mem.h>

static struct list_head ranges;

typedef struct range_s
{
	struct list_head link;
	uint64_t base;
	uint64_t limit;
} range_t;


static
void range_init(void)
{
	INIT_LIST_HEAD(&ranges);
}


static
range_t *range_alloc(uint64_t base,
		     uint64_t limit)
{
	range_t *r = mem_malloc(sizeof(range_t));
	BUG_ON(r == NULL, "failed to alloc range struct for 0x%lx-0x%lx",
	       base, limit);

	r->base = base;
	r->limit = limit;
	INIT_LIST_HEAD(&r->link);
	return r;
}


static
void range_dump(void)
{
	range_t *range;

	list_for_each_entry(range, &ranges, link) {
		LOG("range 0x%lx-0x%lx", range->base, range->limit);
	}
}


static
void range_add(uint64_t base,
	       uint64_t limit)
{
	range_t *range;
	range_t *r = range_alloc(base, limit);

	BUG_ON(base >= limit, "base (0x%lx) >= limit (0x%lx)",
	       base, limit);

	list_for_each_entry(range, &ranges, link) {
		BUG_ON(base >= range->base && limit <= range->limit,
		       "range 0x%lx-0x%lx already present",
		       base, limit);

		if (range->base > limit) {
			list_add_tail(&r->link, &range->link);
			return;
		}
	}

	list_add_tail(&r->link, &ranges);
}


static
void range_remove(uint64_t base,
		  uint64_t limit)
{
	range_t *range;
	range_t *n;

	BUG_ON(base >= limit, "base (0x%lx) >= limit (0x%lx)",
	       base, limit);

	list_for_each_entry_safe(range, n, &ranges, link) {
		if (range->base > limit ||
		    range->limit < base) {
			continue;
		} else if (range->base >= base &&
		    range->limit <= limit) {
			/*
			 * Entire range to be deleted.
			 */
			list_del(&range->link);
			mem_free(range);
		} else {
			if (range->base >= base) {
				/*
				 * range->limit > limit.
				 */
				range->base = limit + 1;
			} else if (range->limit <= limit) {
				/*
				 * range->base < base.
				 */
				range->limit = base - 1;
			} else {
				uint64_t b = range->base;
				/*
				 * range->base < base.
				 * range->limit > limit.
				 */
				range->base = limit + 1;
				range_add(b, base - 1);
			}
		}
	}
}

static bool_t mem_early = true;
#define EARLY_MEMORY (PAGE_SIZE * 64)
static uint8_t early_memory[EARLY_MEMORY];
static uint8_t *sbrk = early_memory;


err_t
mem_init(void *fdt)
{
	ra_t base;
	size_t size;
	int node = -1;
	int prop_len;
	int resv_count;
	const uint64_t *prop;
	err_t status = ERR_NOT_FOUND;

	LOG("Early memory: 0x%lx - 0x%lx", early_memory,
	     early_memory + EARLY_MEMORY - 1);
	range_init();

	for (node = fdt_node_offset_by_dtype(fdt, -1, "memory");
	     node != -1;
	     node = fdt_node_offset_by_dtype(fdt, node, "memory")) {
		prop = fdt_getprop(fdt, node, "reg", &prop_len);
		BUG_ON(prop == NULL, "node %d has no reg property", node);
		BUG_ON(prop_len != (2 * sizeof(uint64_t)),
		       "node %d reg has unexpected len %u", node, prop_len);

		base = fdt64_to_cpu(*prop);
		size  = fdt64_to_cpu(*(prop + 1));
		LOG("Memory: 0x%lx - 0x%lx", base, base + size - 1);
		range_add(base, base + size - 1);
		status = ERR_NONE;
	}

	if (status != ERR_NONE) {
		return status;
	}

	/*
	 * Remove self.
	 */
	range_remove((uint64_t) ptr_2_ra(&_start), ptr_2_ra(&_end) - 1);

	/*
	 * Remove the CPU exception vectors at 0.
	 */
	range_remove((uint64_t) 0,  (uintptr_t) &exc_end - (uintptr_t) &exc_base - 1);

	/*
	 * Remove fdt itself (should I rely on this being one of OPAL
	 * reserved ranges?).
	 */
	range_remove(ptr_2_ra(fdt), ptr_2_ra(fdt) + fdt_size_dt_struct(fdt) - 1);

	resv_count = fdt_num_mem_rsv(fdt);
	for (node = 0; node < resv_count; node++) {
		fdt_get_mem_rsv(fdt, node, &base, &size);
		LOG("Reserved: 0x%lx - 0x%lx", base, base + size - 1);
		range_remove(base, base + size - 1);
	}


	node = fdt_path_offset(fdt, "/chosen");
	BUG_ON(node == -1, "/chosen not found");

	prop = fdt_getprop(fdt, node, "linux,initrd-start", &prop_len);
	BUG_ON(prop == NULL, "node %d has no linux,initrd-start property",
	       node);
	BUG_ON(prop_len != sizeof(uint32_t) &&
	       prop_len != sizeof(uint64_t),
	       "node %d reg has unexpected len %u", node, prop_len);

	if (prop_len == sizeof(uint32_t)) {
		base = fdt32_to_cpu(* (uint32_t *) prop);
	} else {
		base = fdt64_to_cpu(*prop);
	}

	prop = fdt_getprop(fdt, node, "linux,initrd-end", &prop_len);
	BUG_ON(prop == NULL, "node %d has no linux,initrd-end property",
	       node);
	BUG_ON(prop_len != sizeof(uint32_t) &&
	       prop_len != sizeof(uint64_t),
	       "node %d reg has unexpected len %u", node, prop_len);

	if (prop_len == sizeof(uint32_t)) {
		size = fdt32_to_cpu(* (uint32_t *) prop) - base;
	} else {
		size = fdt64_to_cpu(*prop) - base;
	}

	LOG("Initrd: 0x%lx - 0x%lx", base, base + size - 1);
	range_remove(base, base + size - 1);

	mem_early = false;
	range_dump();
	return status;
}


int
mem_put_pages(void *base,
              size_t length)
{
	FATAL("not expected");
	return 0;
}


void *
mem_get_pages(size_t length)
{
	range_t *r;
	range_t *n;
	BUG_ON(length == 0, "length cannot be 0", length);
	BUG_ON(length % PAGE_SIZE, "length must be multiple of PAGE_SIZE", length);

	if (mem_early) {
		sbrk = PALIGN_UP(sbrk, PAGE_SIZE) + length;
		BUG_ON(sbrk - early_memory > sizeof(early_memory),
		       "too much early memory used");
		return sbrk - length;
	}

	list_for_each_entry_safe(r, n, &ranges, link) {
		ra_t base_aligned = ALIGN_UP(r->base, PAGE_SIZE);
		ra_t limit_aligned = ALIGN(r->limit, PAGE_SIZE);

		if (base_aligned < limit_aligned &&
		    (limit_aligned - base_aligned) >= length) {
			range_remove(base_aligned, base_aligned + length - 1);
			return ra_2_ptr(base_aligned);
		}
	}

	BUG_ON(NULL, "no more free memory");
	return NULL;
}
