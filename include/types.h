/*
 * Basic types.
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

#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long int64_t;
typedef uint64_t uintptr_t;
typedef int64_t intptr_t;
typedef uint64_t __be64;

typedef uint64_t vaddr_t;
typedef uint64_t offset_t;
typedef uint64_t length_t;
typedef uint64_t size_t;
typedef uint64_t count_t;

typedef _Bool bool_t;

#define USHRT_MAX       ((uint16_t)(~0U))
#define SHRT_MAX        ((int16_t)(USHRT_MAX>>1))
#define SHRT_MIN        ((int16_t)(-SHRT_MAX - 1))
#define INT_MAX         ((int)(~0U>>1))
#define INT_MIN         (-INT_MAX - 1)
#define UINT_MAX        (~0U)
#define LONG_MAX        ((long)(~0UL>>1))
#define LONG_MIN        (-LONG_MAX - 1)
#define ULONG_MAX       (~0UL)

/* System address, RA and EA. */
typedef uint64_t ra_t;
typedef uint64_t ea_t;

/* Guest addreses, RA and EA. */
typedef uint64_t gra_t;
typedef uint64_t gea_t;

#define ERR_LIST							\
	ERR_DEF(ERR_NONE, "no error")					\
	ERR_DEF(ERR_NOT_READY, "not ready")				\
	ERR_DEF(ERR_UNSUPPORTED, "not supported")			\
	ERR_DEF(ERR_NO_MEM, "no memory")				\
	ERR_DEF(ERR_NOT_FOUND, "not found")				\
	ERR_DEF(ERR_INVALID, "invalid error, likely a bug") /* last */

#define ERR_DEF(e, s) e,
typedef enum {
  ERR_LIST
} err_t;
#undef ERR_DEF

#endif /* TYPES_H */
