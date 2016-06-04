/*
 * OF emulation, enough for the ARC veneer anyway.
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

#ifndef ROM_H
#define ROM_H

#include <types.h>
#include <exc.h>

typedef struct rom_t
{
	gra_t cif_trampoline;
	gra_t claim_arena_start;
	gra_t claim_arena_ptr;
	gra_t claim_arena_end;
	gra_t stack_start;
	gra_t stack_end;
} rom_t;

err_t rom_init(void *fdt);
err_t rom_call(eframe_t *frame);

#endif /* ROM_H */
