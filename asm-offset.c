/*
 * C offsets for assembler code.
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

#include <kpcr.h>
#include <stddef.h>

#define DEFINE(sym, val) \
	asm volatile("\n->" #sym " %0 " #val : : "i" (val))

#define OFFSET(sym, str, mem)			\
	DEFINE(sym, offsetof(struct str, mem))

void
unused(void)
{
	OFFSET(kpcr_toc, kpcr_s, toc);
	OFFSET(kpcr_opal_base, kpcr_s, opal_base);
	OFFSET(kpcr_opal_entry, kpcr_s, opal_entry);
	OFFSET(kpcr_slb_size, kpcr_s, slb_size);
}
