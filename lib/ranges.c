/*
 * Range-based resource tracking.
 *
 * Copyright (C) 2016 Andrei Warkentin <andrey.warkentin@gmail.com>
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

#include <defs.h>
#include <assert.h>
#include <ranges.h>
#include <mem.h>


length_t
range_count(ranges_t *ranges)
{
	range_t *range;
	length_t c = 0;

	list_for_each_entry(range, ranges, link) {
		c++;
	}

	return c;
}


range_t *
range_alloc(uint64_t base,
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


void
range_dump(ranges_t *ranges)
{
	range_t *range;

	list_for_each_entry(range, ranges, link) {
		LOG("range 0x%lx-0x%lx", range->base, range->limit);
	}
}


void
range_add(ranges_t *ranges,
	  uint64_t base,
	  uint64_t limit)
{
	range_t *range;
	range_t *r = range_alloc(base, limit);

	BUG_ON(base >= limit, "base (0x%lx) >= limit (0x%lx)",
	       base, limit);

	list_for_each_entry(range, ranges, link) {
		BUG_ON(base >= range->base && limit <= range->limit,
		       "range 0x%lx-0x%lx already present",
		       base, limit);

		if (range->base > limit) {
			list_add_tail(&r->link, &range->link);
			return;
		}
	}

	list_add_tail(&r->link, ranges);
}


void
range_remove(ranges_t *ranges,
	     uint64_t base,
	     uint64_t limit)
{
	range_t *range;
	range_t *n;

	BUG_ON(base >= limit, "base (0x%lx) >= limit (0x%lx)",
	       base, limit);

	list_for_each_entry_safe(range, n, ranges, link) {
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
				range_add(ranges, b, base - 1);
			}
		}
	}
}
