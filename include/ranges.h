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

#ifndef RANGE_H
#define RANGE_H

#include <types.h>
#include <list.h>

typedef struct list_head ranges_t;

typedef struct range_s {
	struct list_head link;
	uint64_t base;
	uint64_t limit;
} range_t;

static inline
void range_init(ranges_t *ranges)
{
	INIT_LIST_HEAD(ranges);
}

range_t *range_alloc(uint64_t base, uint64_t limit);
void range_dump(ranges_t *ranges);
void range_add(ranges_t *ranges,
	       uint64_t base,
	       uint64_t limit);
void range_remove(ranges_t *ranges,
		  uint64_t base,
		  uint64_t limit);

#endif /* RANGE_H */
