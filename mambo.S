/*
 * Callthru (i.e. semihosting) facilities.
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

/*
 * Use simulator callthru (i.e. semihosting) facilities
 * to print a string.
 *
 * r3 - buffer
 * r4 - len
 */
_GLOBAL(mambo_write)
#define SIM_WRITE_CONSOLE_CODE 0
	li	r6, 0
	mr	r5, r4
	mr	r4, r3
	li	r3, SIM_WRITE_CONSOLE_CODE
	.long	0x000eaeb0
	blr
