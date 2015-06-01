/*
 * Various PPC accessors.
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

#ifndef PPC_H
#define PPC_H

#include <types.h>
#include <ppc-defs.h>
#include <defs.h>


static inline void eieio(void)
{
	asm volatile("eieio" : : : "memory");
}


static inline void
ptesync(void)
{
	asm volatile ("ptesync" : : : "memory");
}


static inline void
tlbsync(void)
{
	asm volatile ("tlbsync" : : : "memory");
}


static inline void
lwsync(void)
{
	asm volatile ("lwsync" : : : "memory");
}


static inline void
isync(void)
{
	asm volatile ("isync" : : : "memory");
}

#define REG_READ_FN(reg)						\
	static inline uint64_t						\
	get_##reg(void)							\
	{								\
		uint64_t reg = 0;					\
		asm volatile("mfspr %0, " S(SPRN_##reg)			\
			     : "=r" (reg) :: "memory");			\
		return reg;						\
	}								\

#define REG_WRITE_FN(reg)						\
	static inline void						\
	set_##reg(uint64_t reg)						\
	{								\
		asm volatile("mtspr "S(SPRN_##reg)", %0"		\
			     :: "r" (reg) : "memory");			\
	}								\

REG_READ_FN(LPCR)
REG_WRITE_FN(LPCR)
REG_READ_FN(HID0)
REG_WRITE_FN(HID0)
REG_WRITE_FN(HSPRG0)
REG_READ_FN(DEC)
REG_WRITE_FN(DEC)
REG_READ_FN(HDEC)
REG_WRITE_FN(HDEC)
REG_READ_FN(SDR1)
REG_WRITE_FN(SDR1)
REG_READ_FN(DAR)
REG_READ_FN(DSISR)


static inline uint64_t
mftb(void)
{
	uint64_t tb;
	asm volatile("mftb %0" : "=r"(tb) : : "memory");
	return tb;
}


static inline uint64_t
mfmsr(void)
{
	uint64_t val;

	asm volatile("mfmsr %0" : "=r"(val) : : "memory");
	return val;
}


static inline void
mtmsr(uint64_t val)
{
	asm volatile("mtmsr %0" : : "r"(val) : "memory");
}


static inline void  __alwaysinline
mtmsrd(uint64_t val, const int l)
{
	asm volatile("mtmsrd %0,%1" : : "r"(val), "i"(l) : "memory");
}


static inline void smt_low(void)        { asm volatile("or 1,1,1");     }
static inline void smt_medium(void)     { asm volatile("or 2,2,2");     }
static inline void smt_high(void)       { asm volatile("or 3,3,3");     }
static inline void smt_medium_high(void){ asm volatile("or 5,5,5");     }
static inline void smt_medium_low(void) { asm volatile("or 6,6,6");     }
static inline void smt_extra_high(void) { asm volatile("or 7,7,7");     }
static inline void smt_very_low(void)   { asm volatile("or 31,31,31");  }


static inline __nomcount void
cpu_relax(void)
{
	/* Relax a bit to give sibling threads some breathing space */
	smt_low();
	smt_very_low();
	asm volatile("nop; nop; nop; nop\n");
	asm volatile("nop; nop; nop; nop\n");
	asm volatile("nop; nop; nop; nop\n");
	asm volatile("nop; nop; nop; nop\n");
	smt_medium();
}

void flush_cache(vaddr_t base, length_t len);

#endif /* PPC_H */
