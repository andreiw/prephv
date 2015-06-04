/*
 * Various useful macros.
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

#ifndef DEFS_H
#define DEFS_H

#include <types.h>

#define min(x,y) ({				\
			typeof(x) _x = (x);	\
			typeof(y) _y = (y);	\
			(void) (&_x == &_y);	\
			_x < _y ? _x : _y; })

#define max(x,y) ({				\
			typeof(x) _x = (x);	\
			typeof(y) _y = (y);	\
			(void) (&_x == &_y);	\
			_x > _y ? _x : _y; })

#define NULL ((void *) 0)
#define BITS_PER_LONG 64
#define ALIGN_UP(addr, align) (((addr) + (align) - 1) & (~((align) - 1)))
#define PALIGN_UP(p, align) ((typeof(p))(((uintptr_t)(p) + (align) - 1) & (~((align) - 1))))
#define ALIGN(addr, align) (((addr) - 1) & (~((align) - 1)))
#define S(...) _S(__VA_ARGS__)
#define _S(...) #__VA_ARGS__

#define MB(x) (1UL * x * 1024 * 1024)
#define TB(x) (1UL * x * 1024 * 1024 * 1024 * 1024)

#define __packed                __attribute__((packed))
#define __align(x)              __attribute__((__aligned__(x)))
#define __unused                __attribute__((unused))
#define __used                  __attribute__((used))
#define __section(x)            __attribute__((__section__(x)))
#define __noreturn              __attribute__((noreturn))
#define __attrconst             __attribute__((const))
#define __warn_unused_result    __attribute__((warn_unused_result))
#define __nomcount              __attribute__((no_instrument_function))
#define __alwaysinline          __attribute__((always_inline))


/*
 * Return the zero-based bit position (LE, not IBM bit numbering) of
 * the most significant 1-bit in a double word.
 */
static __inline__ __attribute__((const))
int __ilog2(uint64_t x)
{
	int lz;

	asm ("cntlzd %0,%1" : "=r" (lz) : "r" (x));
	return BITS_PER_LONG - 1 - lz;
}
#endif /* DEFS_H */
