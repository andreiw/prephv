/*
 * Endian handling.
 * Most of this code is derived from the Linux Kernel.
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

#ifndef ENDIAN_H
#define ENDIAN_H

#include <types.h>


static inline uint16_t
swab16(uint16_t x)
{
	return  ((x & (uint16_t)0x00ffU) << 8) |
		((x & (uint16_t)0xff00U) >> 8);
}


static inline uint32_t
swab32(uint32_t x)
{
	return  ((x & (uint32_t)0x000000ffUL) << 24) |
		((x & (uint32_t)0x0000ff00UL) <<  8) |
		((x & (uint32_t)0x00ff0000UL) >>  8) |
		((x & (uint32_t)0xff000000UL) >> 24);
}


static inline uint64_t
swab64(uint64_t x)
{
	return  (uint64_t)((x & (uint64_t)0x00000000000000ffULL) << 56) |
		(uint64_t)((x & (uint64_t)0x000000000000ff00ULL) << 40) |
		(uint64_t)((x & (uint64_t)0x0000000000ff0000ULL) << 24) |
		(uint64_t)((x & (uint64_t)0x00000000ff000000ULL) <<  8) |
		(uint64_t)((x & (uint64_t)0x000000ff00000000ULL) >>  8) |
		(uint64_t)((x & (uint64_t)0x0000ff0000000000ULL) >> 24) |
		(uint64_t)((x & (uint64_t)0x00ff000000000000ULL) >> 40) |
		(uint64_t)((x & (uint64_t)0xff00000000000000ULL) >> 56);
}

#define cpu_to_be64(x) swab64(x)
#define cpu_to_be32(x) swab32(x)
#define cpu_to_be16(x) swab16(x)
#define cpu_to_le64(x) ((uint64_t) x)
#define cpu_to_le32(x) ((uint32_t) x)
#define cpu_to_le16(x) ((uint16_t) x)
#define be64_to_cpu(x) swab64(x)
#define be32_to_cpu(x) swab32(x)
#define be16_to_cpu(x) swab16(x)
#define le64_to_cpu(x) ((uint64_t) x)
#define le32_to_cpu(x) ((uint32_t) x)
#define le16_to_cpu(x) ((uint16_t) x)

#endif /* ENDIAN_H */
