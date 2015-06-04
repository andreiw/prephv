/*
 * Basic assert. Not really nice.
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

#ifndef ASSERT_H
#define ASSERT_H

#include <console.h>
#include <ppc.h>

#define BUG_ON(x) do {					\
		if ((x)) {				\
			mtmsrd(0, 1);			\
			printk("BUG: %s", S(x));	\
			while(1);			\
		}					\
	} while(0);

#endif /* ASSERT_H */
