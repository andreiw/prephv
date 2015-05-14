/*
 * Various console-related bits.
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

#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdarg.h>

#define OPAL_TERMINAL_0 0

void opal_write(int terminal, u64 *len, char *buf);
void mambo_write(char *buf, int len);
void printk(char *fmt, ...);
void vprintk(char *fmt, va_list adx);

#endif /* CONSOLE_H */
