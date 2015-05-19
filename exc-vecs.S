/*
 * Exception handlers.
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

#include <asm-utils.h>
#include <ppc-defs.h>
#include <opal.h>
#include <kpcr.h>

.p2align 8
.globl exc_base
exc_base:
	rfid

#define DEF_EXC(name)				\
	.org exc_base + EXC_##name;		\
	.globl exc_##name;			\
	exc_##name:				\
	b exc_base;				\

DEF_EXC(RESET)
DEF_EXC(MC)
DEF_EXC(DSI)
DEF_EXC(DSEG)
DEF_EXC(ISI)
DEF_EXC(ISEG)
DEF_EXC(EXT)
DEF_EXC(ALIGN)
DEF_EXC(PROG)
DEF_EXC(FPU)
DEF_EXC(DEC)
DEF_EXC(HDEC)
DEF_EXC(DOOR)
DEF_EXC(RESV)
DEF_EXC(SC)
DEF_EXC(TRACE)
DEF_EXC(HDSI)
DEF_EXC(HISI)
DEF_EXC(HEA)
DEF_EXC(HMAINT)
DEF_EXC(HDOOR)
DEF_EXC(RESV1)
DEF_EXC(RESV2)
DEF_EXC(IMPL)
DEF_EXC(PMU)
DEF_EXC(VECUN)
DEF_EXC(VSX)
DEF_EXC(FAC)
DEF_EXC(HFAC)
.org exc_base + EXC_TABLE_END
.globl exc_table_end
exc_table_end:
