/*
 * Dumb printing routines
 *
 * Copyright (C) Andrey Warkentin <andrey.warkentin@gmail.com>
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
#include <stdarg.h>
#include <defs.h>
#include <endian.h>
#include <console.h>
#include <types.h>
#include <opal.h>
#include <vsprintf.h>


static void
_putchar(char c)
{
	uint64_t len = cpu_to_be64(1);
	opal_write(OPAL_TERMINAL_0, ptr_2_ra(&len), ptr_2_ra(&c));
}


void
con_putchar(char c)
{
	_putchar(c);
	if (c == '\n') {
		_putchar('\r');
	}
}


void
con_puts(char *s)
{
	while (*s != '\0') {
		con_putchar(*s++);
	}
}


void
con_puts_len(char *s,
	     length_t len)
{
	len = cpu_to_be64(len);
	opal_write(OPAL_TERMINAL_0, ptr_2_ra(&len), ptr_2_ra(s));
}


int
con_getchar(void)
{
	char c;
	opal_return_t ret;
	uint64_t len = cpu_to_be64(1);

	ret = opal_read(OPAL_TERMINAL_0, ptr_2_ra(&len), ptr_2_ra(&c));
	if (ret != OPAL_SUCCESS || len == 0) {
		return NO_CHAR;
	}

	return c;
}
