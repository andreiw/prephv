/*
 * KPCR and accessors.
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

#ifndef KPCR_H
#define KPCR_H

#ifdef __ASSEMBLY__

#include <asm-offset.h>

.extern kpcr
#define KPCR_R(name,reg) (kpcr_ ## name)(reg)
#define KPCR(name) KPCR_R(name, r13)

#else /* !__ASSEMBLY__ */

#include <types.h>
#include <defs.h>

/*
 * Toy per-cpu structure (for only 1 CPU). Sigh, I keep all
 * sorts of stuff here that really isn't per-CPU :(, however,
 * it's nice to be able to avoid immediate loads and use r13-
 * -based addressing, almost like GOT/TOC addressing, except
 * it is non-volatile in most sane situations. Think of it as
 * the TLS pointer. Also, pointer is kept in HSPRG0 such that
 * we can use it in exception handling. Yes, in this toy
 * example I could just read r13, but it's nice to do things
 * "for real".
 */
typedef struct kpcr_s {
	uint64_t toc;
	uint64_t opal_base;
	uint64_t opal_entry;
	uint64_t slb_size;
	uint64_t unrec_sp;
	uint64_t kern_sp;
	uint64_t tb_freq;
} kpcr_t;


static inline __nomcount kpcr_t *
kpcr_get(void)
{
	register kpcr_t *kpcr asm("r13");
	return kpcr;
}
#endif /* !__ASSEMBLY__ */
#endif /* KPCR_H */
