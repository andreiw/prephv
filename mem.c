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

static uint8_t *base = (uint8_t *) &_end;
intptr_t sbrk_size = 0;


void *
mem_sbrk(intptr_t diff)
{
	BUG_ON((sbrk_size + diff) < 0, "%z makes sbrk_size go under", diff);
	base += diff;
	return base;
}
