/*
 * String ops.
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

#define NULL ((void *) 0)
#define ALIGN_UP(addr, align) (((addr) + (align) - 1) & (~((align) - 1)))
#define PALIGN_UP(p, align) ((typeof(p))(((uintptr_t)(p) + (align) - 1) & (~((align) - 1))))
#define ALIGN(addr, align) (((addr) - 1) & (~((align) - 1)))
#define S(x) _S(x)
#define _S(x) #x

#define __packed                __attribute__((packed))
#define __align(x)              __attribute__((__aligned__(x)))
#define __unused                __attribute__((unused))
#define __used                  __attribute__((used))
#define __section(x)            __attribute__((__section__(x)))
#define __noreturn              __attribute__((noreturn))
#define __attrconst             __attribute__((const))
#define __warn_unused_result    __attribute__((warn_unused_result))

#endif /* DEFS_H */
