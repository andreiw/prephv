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
#define ALIGN_UP_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define ALIGN_MASK(x, mask)    ((x) & ~(mask))
#define ALIGN(x, a)            ALIGN_MASK(x, (typeof(x))(a) - 1)
#define IS_ALIGNED(x, a)       (((x) & ((typeof(x))(a) - 1)) == 0)
#define ALIGN_UP(x, a)         (IS_ALIGNED(x, a) ? (x) : ALIGN_UP_MASK(x, (typeof(x))(a) - 1))

#define PALIGN(p, align)       ((typeof(p)) ALIGN((uintptr_t)(p),       \
                                                  (uintptr_t) (align))
#define PALIGN_UP(p, align)    ((typeof(p)) ALIGN_UP((uintptr_t)(p),    \
                                                     (uintptr_t)(align)))
#define SIFY(...) _SIFY(__VA_ARGS__)
#define _SIFY(...) #__VA_ARGS__

/* Mask off and return a bit slice. */
#define MASK_OFF(value, larger, smaller) (((value) >> (smaller)) & ((1UL << ((larger) + 1UL - (smaller))) - 1UL))

/* Form a bit slice. */
#define MASK_IN(value, larger, smaller) (((value) & ((1UL << ((larger) + 1UL - (smaller))) - 1UL)) << (smaller))

#define __FORCE_64(x) x ## UL
#define ARRAY_LEN(x) (sizeof((x)) / sizeof((x)[0]))

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#define offsetof(TYPE, MEMBER) ((length_t) &((TYPE *)0)->MEMBER)

#define MB(x) (1UL * x * 1024 * 1024)
#define GB(x) (1UL * x * 1024 * 1024 * 1024)
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

#define likely(x)     (__builtin_constant_p(x) ? !!(x) : __builtin_expect(!!(x), 1))
#define unlikely(x)   (__builtin_constant_p(x) ? !!(x) : __builtin_expect(!!(x), 0))

#define do_div(n,base) ({\
  uint32_t __base = (base);\
  uint32_t __rem;\
  __rem = ((uint64_t)(n)) % __base;\
  (n) = ((uint64_t)(n)) / __base;\
  __rem;\
    })

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
