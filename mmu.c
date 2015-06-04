/*
 * HTAB/SLB management. Based on the Linux hash_native_64.c logic,
 * mostly, although simplified to the point where it's only good
 * for getting a basic understanding, not for actually using it
 * anywhere ;-).
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

#include <types.h>
#include <defs.h>
#include <assert.h>
#include <console.h>
#include <linkage.h>
#include <endian.h>
#include <string.h>
#include <assert.h>
#include <kpcr.h>
#include <ppc.h>
#include <mmu.h>
#include <mem.h>

/*
 * in order to fit the 78 bit va in a 64 bit variable we shift the va by
 * 12 bits. This enable us to address upto 76 bit va.
 * For hpt hash from a va we can ignore the page size bits of va and for
 * hpte encoding we ignore up to 23 bits of va. So ignoring lower 12 bits ensure
 * we work in all cases including 4k page size.
 */
#define VPN_SHIFT 12
typedef uint64_t vpn_t;


static inline vpn_t
ea_2_vpn(ea_t ea)
{
	/*
	 * ESID(800000) => VSID(0).
	 * ESID(000000) => VSID(0).
	 */
	if ((ea & HV_ASPACE) != 0) {
		return (ea & ~HV_ASPACE) >> VPN_SHIFT;
	}

	return ea >> VPN_SHIFT;
}

typedef struct {
	__be64 v;
	__be64 r;
} pte_t;

static void *htab;
static count_t ptegs;
static length_t htab_size;
static uint64_t htab_hash_mask;


static uint64_t
slb_make_esid(ea_t ea, uint64_t slot)
{
	/*
	 * 1TB.
	 */
	return (ea & ESID_MASK_1T) | SLB_ESID_V | slot;
}


static uint64_t
slb_make_vsid(uint64_t vsid)
{
	/*
	 * Supervisor, 1TB.
	 */
	return (vsid << SLB_VSID_SHIFT_1T) |
		SLB_VSID_B_1T |
		SLB_VSID_KP;
}


static void
slb_dump(void)
{
	int entry;
	uint64_t esid;
	uint64_t vsid;

	printk("SLB entries:\n");
	for (entry = 0; entry < kpcr_get()->slb_size; ++entry) {
		asm volatile("slbmfee  %0,%1" : "=r" (esid) : "r" (entry));
		if (esid == 0) {
			/*
			 * Valid bit is clear along with everything else.
			 */
			continue;
		}

		asm volatile("slbmfev  %0,%1" : "=r" (vsid) : "r" (entry));
		printk("%d: E 0x%x V 0x%x\n", entry, esid, vsid);
	}
}


static void
slb_init(void)
{
	uint64_t esid;
	uint64_t vsid;

	asm volatile("slbia" ::: "memory");
	isync();

	/*
	 * 1TB segment at EA=HV_ASPACE => VA=0. We use slot = 1,
	 * since slot 0 has special handling (not invalidated
	 * with tlbia).
	 *
	 * Also create a 1TB segment at EA=0 => VA=0.
	 */
	esid = slb_make_esid(HV_ASPACE, 1);
	vsid = slb_make_vsid(0);
	asm volatile("slbmte %0, %1" ::
		     "r"(vsid), "r"(esid) : "memory");
	esid = slb_make_esid(0, 2);
	vsid = slb_make_vsid(0);
	asm volatile("slbmte %0, %1" ::
		     "r"(vsid), "r"(esid) : "memory");
	isync();

	slb_dump();
}


static inline uint64_t
pteg_count(uint64_t ram_size)
{
	uint64_t ptegs;

	/*
	 * Number of maximum mapped physical pages / 2, as suggested
         * by 5.7.7.1 PowerISA v2.07 p901.
	 */
	ptegs = ram_size >> (PAGE_SHIFT + 1);
	ptegs = max(ptegs, (uint64_t) MIN_PTEGS);
	ptegs = min(ptegs, (uint64_t) MAX_PTEGS);
	return ptegs;
}


static inline uint64_t
vpn_hash(vpn_t vpn,
	 page_size_t base)
{
	int shift;
	vpn_t mask, hash, vsid;

	BUG_ON(base != PAGE_4K, "only 4K base page size supported");
	shift = PAGE_SHIFT;

	mask = (1ul << (SID_SHIFT_1T - VPN_SHIFT)) - 1;
	vsid = vpn >> (SID_SHIFT_1T - VPN_SHIFT);
	hash = vsid ^ (vsid << 25) ^
		((vpn & mask) >> (shift - VPN_SHIFT));

	return hash & 0x7fffffffffUL;
}


static inline uint64_t
rpn_encode(ra_t ra,
	   page_size_t actual)
{
	ra_t lp;
	ra_t mask;

	if (actual == PAGE_16M) {
		lp = PTE_R_4K_16M << PTE_R_LP_SHIFT;
		mask = PTE_R_RPN_16M;
	} else {
		BUG_ON(actual != PAGE_4K, "unsupported actual page size");

		lp = 0;
		mask = PTE_R_RPN_4K;
	}

	/*
	 * ra must be valid and well aligned.
	 */
	BUG_ON((ra & mask) != ra, "ra 0x%x does not match mask 0x%x", ra, mask);

	ra &= mask;
	ra |= lp;

	return ra;
}


static inline uint64_t
avpn_encode(vpn_t vpn,
	    page_size_t actual)
{
	uint64_t v;

	/*
	 * If the base page size goes over 23 bits, those bits need be
	 * cleared. So for 16M pages, we need to clear bit 24.
	 *
	 * For us the base page size is always 4K.
	 */

	/*
	 * The AVA field omits the low-order 23 bits of the 78 bits VA.
	 * These bits are not needed in the PTE, because the
	 * low-order b of these bits are part of the byte offset
	 * into the virtual page and, if b < 23, the high-order
	 * 23-b of these bits are always used in selecting the
	 * PTEGs to be searched.
	 */
	v = vpn >> (23 - VPN_SHIFT);
	v <<= PTE_V_AVPN_SHIFT;
	return v;
}


static inline void
tlbie(vpn_t vpn,
      page_size_t actual)
{
	uint64_t va = vpn << VPN_SHIFT;
	int shift;

	if (actual == PAGE_16M) {
		shift = PAGE_SHIFT_16M;
	} else {
		BUG_ON(actual != PAGE_4K, "unsupported actual page size");
		/*
		 * 4K.
		 */
		shift = PAGE_SHIFT;
	}

	/*
	 * 5.9.3.3 PowerISA v2.07 p928.
	 */
	va &= ~((1ul << shift) - 1);
	va |= TLBIE_RB_1TB;

	if (actual == PAGE_16M) {
		va |= TLBIE_RB_L;
		/*
		 * 16M pages.
		 */
		va |= (PTE_R_4K_16M << TLBIE_RB_LP_SHIFT);

		/*
		 * The AVAL bits represent those bits that end
		 * getting overlapped by LP. Unneeded bits must
		 * be ignored by the CPU. See p928.
		 */
		va |= (vpn & 0xfe);
	}

	asm volatile (PPC_TLBIE(%1, %0) : : "r"(va), "r"(0));
	eieio();
	tlbsync();
	ptesync();
}


void
mmu_unmap(ea_t ea,
	  page_size_t actual)
{
	length_t actual_size;

	if (actual == PAGE_16M) {
		actual_size = PAGE_SIZE_16M;
	} else {
		BUG_ON(actual != PAGE_4K, "unsupported actual page size");
		actual_size = PAGE_SIZE;
	}

	BUG_ON((ea & ~(actual_size - 1)) != ea,
	       "ea 0x%x not aligned to 0x%x", ea, actual_size);

	while (actual_size != 0) {
		int i;
		vpn_t vpn = ea_2_vpn(ea);
		uint64_t hash = vpn_hash(vpn, PAGE_4K);
		uint64_t pteg = ((hash & htab_hash_mask) * PTES_PER_GROUP);
		pte_t *pte = ((pte_t *) htab) + pteg;

		/*
		 * We don't do secondary PTEGs and we're not SMP safe:
		 * real OSes use one of the software bits inside the V
		 * part of pte_t as a spinlock.
		 */
		for (i = 0; i < PTES_PER_GROUP; i++, pte++) {
			if ((be64_to_cpu(pte->v) & PTE_V_VALID) == 0) {
				/*

				 * Not used, go to next slot.
				 */
				continue;
			}

			if (PTE_V_COMPARE(be64_to_cpu(pte->v),
					  avpn_encode(vpn, actual))) {
				break;
			}
		}

		/*
		 * printk("EA 0x%x -> hash 0x%x -> pteg 0x%x "
		 *        "(i = %u v = 0x%x r = 0x%x) = unmap\n",
		 *        ea, hash, pteg, i, be64_to_cpu(pte->v), be64_to_cpu(pte->r));
		 */

		BUG_ON(i == PTES_PER_GROUP, "EA 0x%x not mapped", ea);

		pte->v = be64_to_cpu(0);
		ptesync();
		tlbie(vpn, actual);

		/*
		 * Update the next PTE that is part of the large PTE group.
		 */
		actual_size -= PAGE_SIZE;
		ea += PAGE_SIZE;
	}
}


void
mmu_map(ea_t ea,
	ra_t ra,
	prot_t pp,
	page_size_t actual)
{
	length_t actual_size;
	uint64_t rflags = pp | PTE_R_M;
	uint64_t vflags = PTE_V_1TB_SEG | PTE_R_M | PTE_V_VALID |
		((actual != PAGE_4K) ? PTE_V_LARGE : 0);

	if (actual == PAGE_16M) {
		actual_size = PAGE_SIZE_16M;
	} else {
		BUG_ON(actual != PAGE_4K, "unsupported actual page size");
		actual_size = PAGE_SIZE;
	}

	BUG_ON((ea & ~(actual_size - 1)) != ea,
	       "EA 0x%x not aligned to 0x%x", ea, actual_size);
	BUG_ON((ra & ~(actual_size - 1)) != ra,
	       "RA 0x%x not aligned to 0x%x", ra, actual_size);

	/*
	 * In a loop, with ea incrementing by PAGE_SIZE, to accomodate
	 * large pages in a mixed page environment. The base page size
	 * is 4K.
	 */
	while (actual_size != 0) {
		int i;
		uint64_t v;
		uint64_t r;
		uint64_t hash = vpn_hash(ea_2_vpn(ea), PAGE_4K);
		uint64_t pteg = ((hash & htab_hash_mask) * PTES_PER_GROUP);
		pte_t *pte = ((pte_t *) htab) + pteg;

		/*
		 * We don't do secondary PTEGs and we're not SMP safe:
		 * real OSes use one of the software bits inside the V
		 * part of pte_t as a spinlock.
		 */
		for (i = 0; i < PTES_PER_GROUP; i++, pte++) {
			if ((be64_to_cpu(pte->v) & PTE_V_VALID) != 0) {
				/*
				 * Busy, go to next slot.
				 */
				continue;
			}

			break;
		}

		/*
		 * printk("EA 0x%x -> hash 0x%x -> pteg 0x%x (i = %u) = RA 0x%x\n",
		 * ea, hash, pteg, i, ra);
		 */

		BUG_ON(i == PTES_PER_GROUP, "PTEG spill for EA 0x%x", ea);

		v = avpn_encode(ea_2_vpn(ea), actual) | vflags;
		r = rpn_encode(ra, actual) | rflags;

		/*
		 * See 5.7.3.5 PowerISA v2.07 p895.
		 *
		 * Implicit accesses to the Page Table during address
		 * translation and in recording reference and change infor-
		 * mation are performed as though the storage occupied
		 * by the Page Table had the following storage control
		 * attributes.
		 * W - not Write Through Required
		 * I - not Caching Inhibited
		 * M - Memory Coherence Required
		 * G - not Guarded
		 * not SAO
		 *
		 *... this has the implication that there is no need
		 * to dcbf the HTAB updates themselves. Note that
		 * changes to W/I flags of a mapped page will require
		 * cache maintenance. See 5.8.2.2 PowerISA v2.07 p915.
		 *
		 * For PTE update rules see 5.10.1 PowerISA v2.07 p935.
		 */
		pte->r = cpu_to_be64(r);
		eieio();
		pte->v = cpu_to_be64(v);
		ptesync();

		/*
		 * Update the next PTE that is part of the large PTE group.
		 */
		actual_size -= PAGE_SIZE;
		ea += PAGE_SIZE;
	}
}


void
mmu_map_range(ea_t ea_start,
	      ea_t ea_end,
	      ra_t ra_start,
	      prot_t prot,
	      page_size_t actual)
{
	ea_t addr = ea_start;
	ra_t ra = ra_start;
	length_t size = (actual == PAGE_16M) ? PAGE_SIZE_16M : PAGE_SIZE;

	for (; addr < ea_end; addr += size, ra += size) {
		mmu_map(addr, ra, prot, actual);
	}
}


void
mmu_init(length_t ram_size)
{
	ra_t htab_ra;
	ptegs = pteg_count(ram_size);
	htab_size = ptegs * PTEG_SIZE;
	htab_hash_mask = ptegs - 1;

	/*
	 * This requires Relaxed Page Table Alignment.
	 *
	 * See 5.7.7.4 PowerISA v2.07 p904.
	 */
	htab = mem_alloc(htab_size, HTAB_ALIGN);
	printk("HTAB (%u ptegs, mask 0x%x, size 0x%x) @ %p\n",
	       ptegs, htab_hash_mask, htab_size, htab);

	memset(htab, 0, htab_size);

	/*
	 * The HTABSIZE field in SDR1 contains an integer giving
	 * the number of bits (in addition to the minimum of 11
	 * bits) from the hash that are used in the Page Table
	 * index. This number must not exceed 28.
	 */
	htab_ra = ptr_2_ra(htab);
	BUG_ON((htab_ra & SDR1_MASK) != htab_ra,
	       "HTAB address 0x%x spills outside SDR1");
	set_SDR1(htab_ra + __ilog2(ptegs) - 11);
	slb_init();

	/*
	 * Only map the bare minimum.
	 *
	 * Loaded at 0x00000000200XXXXX.
	 * We run at 0x80000000200XXXXX.
	 *
	 */
	mmu_map_range((ea_t) htab,
		      ((ea_t) htab) + htab_size,
		      htab_ra,
		      PP_RWXX,
		      FALSE);
	mmu_map_range((ea_t) &_start,
		      (ea_t) &_end,
		      ptr_2_ra(&_start),
		      PP_RWXX,
		      FALSE);
}


void
mmu_enable(void)
{
	mtmsr(mfmsr() | MSR_IR | MSR_DR);
}


void
mmu_disable(void)
{
	mtmsr(mfmsr() & ~(MSR_IR | MSR_DR));
}


bool_t
mmu_enabled(void)
{
	return (mfmsr() & (MSR_IR | MSR_DR)) == (MSR_IR | MSR_DR);
}
