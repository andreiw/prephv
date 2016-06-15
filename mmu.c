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
#include <log.h>
#include <linkage.h>
#include <endian.h>
#include <string.h>
#include <assert.h>
#include <kpcr.h>
#include <ppc.h>
#include <mmu.h>
#include <guest.h>
#include <layout.h>
#include <mem.h>

/*
 * in order to fit the 78 bit va in a 64 bit variable we shift the va by
 * 12 bits. This enable us to address up to 76 bit va.
 * For hpt hash from a va we can ignore the page size bits of va and for
 * hpte encoding we ignore up to 23 bits of va. So ignoring lower 12 bits ensure
 * we work in all cases including 4k page size.
 */
#define VPN_SHIFT 12
typedef uint64_t vpn_t;

/*
 * HV/AIL VSIDs need to be large enough not
 * to collide with guest VSIDs.
 *
 * 1T VSIDs are 76 - SID_SHIFT_1T = 36 bits
 * 256M VSIDs are 76 - SID_SHIFT_256MB = 48 bits
 *
 * 32-bit VSIDs are 24 bits large.
 */
#define HV_VSID       0x876543210UL
#define HV_SLOT       0
#define AIL_VSID      0x876543210123UL
#define AIL_SLOT      1
#define GRM_VSID      0x555555555UL
#define GRM_SLOT      2
#define SR_INDEX_TO_SLB_SLOT(x) (x + 3)


static inline seg_size_t
ea_2_seg_size(ea_t ea)
{
	if (ea >= AIL_ASPACE_START &&
	    ea <= AIL_ASPACE_END) {
		return SEG_256M;
	}

	if ((ea & HV_ASPACE) != 0) {
		return SEG_1T;
	}

	BUG_ON(ea >= LAYOUT_VM_END, "unknown EA");
	BUG_ON(guest == NULL, "VM EA access too early");

	if (guest_is_mmu_off()) {
		return SEG_1T;
	}

	return SEG_256M;
}


static inline page_size_t
ea_2_base_size(ea_t ea)
{
	/*
	 * Here we calculate the base page size used to map
	 * particular EAs. The base page size is implicit from
	 * the SLB entry, so this code needs to be kept in sync with
	 * slb_init.
	 *
	 * We need the base page size (not just the actual), because it's
	 * the base page size that is used in finding the right PTEGs.
	 */
	if (ea >= AIL_ASPACE_START &&
	    ea <= AIL_ASPACE_END) {
		return PAGE_4K;
	} else if (ea >= HV_ASPACE) {
		return PAGE_16M;
	}

	BUG_ON(ea >= LAYOUT_VM_END, "unknown EA");
	BUG_ON(guest == NULL, "VM EA access too early");

	if (guest_is_mmu_off()) {
		return PAGE_16M;
	}

	return PAGE_4K;
}


static inline vpn_t
ea_2_vpn(ea_t ea)
{
	int sid_shift;

	if (ea_2_seg_size(ea) == SEG_1T) {
		sid_shift = SID_SHIFT_1T;
	} else {
		sid_shift = SID_SHIFT_256MB;
	}

	/*
	 * AIL space.
	 */
	if (ea >= AIL_ASPACE_START &&
	    ea <= AIL_ASPACE_END) {
		return ((ea - AIL_ASPACE_START) >> VPN_SHIFT) |
			(AIL_VSID << (sid_shift - VPN_SHIFT));
	}

	/*
	 * HV space.
	 */
	if ((ea & HV_ASPACE) != 0) {
		return ((ea & ~HV_ASPACE) >> VPN_SHIFT) |
		       (HV_VSID << (sid_shift - VPN_SHIFT));
	}

	BUG_ON(ea >= LAYOUT_VM_END, "unexpected EA");
	BUG_ON(guest == NULL, "VM EA access too early");

	if (guest_is_mmu_off()) {
		return ((ea - LAYOUT_VM_START) >> VPN_SHIFT) |
			(GRM_VSID << (sid_shift - VPN_SHIFT));
	}

	return ((ea - LAYOUT_VM_START) >> VPN_SHIFT) |
		(guest->sr[SR_INDEX(ea)] << (sid_shift - VPN_SHIFT));
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
slb_make_esid(ea_t ea,
              seg_size_t seg_size,
              uint64_t slot)
{
	/*
	 * 1TB.
	 */
	if (seg_size == SEG_1T) {
		return (ea & ESID_MASK_1T) | SLB_ESID_V | slot;
	} else {
		return (ea & ESID_MASK_256MB) | SLB_ESID_V | slot;
	}
}


static uint64_t
slb_make_vsid(uint64_t vsid,
	      seg_size_t seg_size,
	      page_size_t base)
{
	uint64_t lp;
	uint64_t v = SLB_VSID_KP; /* Supervisor */

	if (seg_size == SEG_1T) {
		v |= (vsid << SLB_VSID_SHIFT_1T) |
			SLB_VSID_B_1T;
	} else {
		v |= (vsid << SLB_VSID_SHIFT_256MB);
	}

	if (base == PAGE_16M) {
		v |= SLB_VSID_L;
		lp = SLB_VSID_LP_16M;
	} else {
		lp = SLB_VSID_LP_4K;
	}

	return v | (lp << SLB_VSID_LP_SHIFT);
}


static void
slb_dump(void)
{
	int entry;
	uint64_t esid;
	uint64_t vsid;

	LOG("SLB entries:");
	for (entry = 0; entry < kpcr_get()->slb_size; ++entry) {
		asm volatile("slbmfee  %0,%1" : "=r" (esid) : "r" (entry));
		if (esid == 0) {
			/*
			 * Valid bit is clear along with everything else.
			 */
			continue;
		}

		asm volatile("slbmfev  %0,%1" : "=r" (vsid) : "r" (entry));
		LOG("%d: E 0x%lx V 0x%lx", entry, esid, vsid);
	}
}


static void
slb_init(void)
{
	uint64_t esid;
	uint64_t vsid;

	asm volatile("slbia");
	isync();

	/*
	 *
	 *  HV: 1TB segment ESID(800000) => 16M pages, slot 0.
	 * AIL: 256M segment ESID(c00000000) => 4K pages, slot 1.
	 *
	 * Slot 0  is special (not invalidated with slbia).
	 */
	esid = slb_make_esid(HV_ASPACE, SEG_1T, HV_SLOT);
	vsid = slb_make_vsid(HV_VSID, SEG_1T, PAGE_16M);
	asm volatile("slbmte %0, %1" ::
		     "r"(vsid), "r"(esid) : "memory");

	esid = slb_make_esid(AIL_ASPACE_START, SEG_256M, AIL_SLOT);
	vsid = slb_make_vsid(AIL_VSID, SEG_256M, PAGE_4K);
	asm volatile("slbmte %0, %1" ::
		     "r"(vsid), "r"(esid) : "memory");
	isync();

	slb_dump();
}


static
void slb_invalidate(ea_t ea,
                    seg_size_t seg_size)
{
	uint64_t ea_mask;
	uint64_t v = 0;

	if (seg_size == SEG_1T) {
		v = SLBIE_B_1T;
		ea_mask = ESID_MASK_1T;
	} else {
		ea_mask = ESID_MASK_256MB;
	}

	v |= ea & ea_mask;
	isync();
	asm volatile("slbie %0\n\t" : : "r" (v));
	isync();
}


err_t
mmu_guest_fault_seg(ea_t ea)
{
	uint64_t esid;
	uint64_t vsid;
	unsigned sr_index = SR_INDEX(ea);
	unsigned slb_slot = SR_INDEX_TO_SLB_SLOT(sr_index);

	BUG_ON(sr_index >= SR_COUNT, "invalid SR index, bad EA 0x%lx", ea);

	asm volatile("slbmfee  %0,%1" : "=r" (esid) :
		     "r" (slb_slot));
	if ((esid & SLB_ESID_V) != 0) {
		/*
		 * Segment is already present...
		 */
		return ERR_UNSUPPORTED;
	}

	/*
	 * mmu_guest_update sets up the GRM segment.
	 */
	BUG_ON(guest_is_mmu_off(), "SLB miss should only happen with guest MMU off");

	esid = slb_make_esid(ea, SEG_256M, slb_slot);
	/*
	 * FIXME: think through Kp/Ks key depending on
	 * guest MSR.PR.
	 */
	vsid = slb_make_vsid(guest->sr[sr_index] & SR_VSID_MASK,
			     SEG_256M, PAGE_4K);
	asm volatile("slbmte %0, %1" ::
		     "r"(vsid), "r"(esid) : "memory");
	isync();

	return ERR_NONE;
}


void
mmu_guest_update(bool mmu_turning_on)
{
	if (mmu_turning_on) {
		slb_invalidate(LAYOUT_VM_START, SEG_1T);
	} else {
		int i;
		uint64_t esid;
		uint64_t vsid;

		for (i = 0; i < SR_COUNT; i++) {
			slb_invalidate(SR_INDEX_TO_EA(i), SEG_256M);
		}

		esid = slb_make_esid(LAYOUT_VM_START, SEG_1T, GRM_SLOT);
		vsid = slb_make_vsid(GRM_VSID, SEG_1T, PAGE_16M);
		asm volatile("slbmte %0, %1" ::
			     "r"(vsid), "r"(esid) : "memory");
		isync();
	}
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
	 seg_size_t seg_size,
	 page_size_t base)
{
	int shift;
	int seg_shift;
	vpn_t mask, hash, vsid;

	BUG_ON(base != PAGE_4K && base != PAGE_16M,
	       "page_size_t %u not supported", base);

	if (seg_size == SEG_1T) {
		seg_shift = SID_SHIFT_1T;
	} else {
		seg_shift = SID_SHIFT_256MB;
	}

	if (base == PAGE_4K) {
		shift = PAGE_SHIFT;
	} else {
		shift = PAGE_SHIFT_16M;
	}

	mask = (1ul << (seg_shift - VPN_SHIFT)) - 1;
	vsid = vpn >> (seg_shift - VPN_SHIFT);
	hash = vsid ^ (vsid << 25) ^
		((vpn & mask) >> (shift - VPN_SHIFT));

	return hash & 0x7fffffffffUL;
}


static inline uint64_t
rpn_encode(ra_t ra,
	   page_size_t base,
	   page_size_t actual)
{
	ra_t lp;
	ra_t mask;

	BUG_ON(base != PAGE_4K && base != PAGE_16M,
		       "page_size_t %u not supported", base);
	BUG_ON(actual != PAGE_4K && actual != PAGE_16M,
		       "page_size_t %u not supported", actual);

	if (actual == PAGE_16M) {
		if (base == PAGE_4K) {
			lp = PTE_R_LP_4K_16M;
		} else {
			lp = PTE_R_LP_16M_16M;
		}

		mask = PTE_R_RPN_16M;
	} else {
		lp = PTE_R_LP_4K_4K;
		mask = PTE_R_RPN_4K;
	}

	/*
	 * ra must be valid and well aligned.
	 */
	BUG_ON((ra & mask) != ra, "RA 0x%x does not match mask 0x%x", ra, mask);

	ra &= mask;
	ra |= (lp << PTE_R_LP_SHIFT);

	return ra;
}


static inline uint64_t
avpn_encode(vpn_t vpn,
	    page_size_t base,
	    page_size_t actual)
{
	uint64_t v;

	BUG_ON(base != PAGE_4K && base != PAGE_16M,
		       "page_size_t %u not supported", base);
	BUG_ON(actual != PAGE_4K && actual != PAGE_16M,
		       "page_size_t %u not supported", actual);

	/*
	 * The AVA field omits the low-order 23 bits of the 78 bits VA.
	 * These bits are not needed in the PTE, because the
	 * low-order b of these bits are part of the byte offset
	 * into the virtual page and, if b < 23, the high-order
	 * 23-b of these bits are always used in selecting the
	 * PTEGs to be searched.
	 */
	v = vpn >> (23 - VPN_SHIFT);

	/*
	 * If the base page size goes over 23 bits, those bits need be
	 * cleared. So for 16M pages, we need to clear bit 24.
	 */
	if (base == PAGE_16M) {
		v &= ~1UL;
	}

	v <<= PTE_V_AVPN_SHIFT;
	return v;
}


static inline void
tlbia(void)
{
	int set;

	for (set = 0; set < TLBIEL_MAX_SETS; set++) {
		uint64_t v = (set << TLBIEL_SET_SHIFT) |
			(TLBIEL_IS_ALL_IN_SET << TLBIEL_IS_SHIFT);
		asm volatile("tlbiel %0" :: "r"(v));
	}

	asm volatile("ptesync");
	asm volatile("isync");
}


static inline void
tlbie(vpn_t vpn,
      seg_size_t seg_size,
      page_size_t base,
      page_size_t actual)
{
	uint64_t va = vpn << VPN_SHIFT;
	int shift;

	BUG_ON(base != PAGE_4K && base != PAGE_16M,
		       "page_size_t %u not supported", base);
	BUG_ON(actual != PAGE_4K && actual != PAGE_16M,
		       "page_size_t %u not supported", actual);

	if (actual == PAGE_16M) {
		shift = PAGE_SHIFT_16M;
	} else {
		shift = PAGE_SHIFT;
	}

	/*
	 * 5.9.3.3 PowerISA v2.07 p928.
	 */
	va &= ~((1ul << shift) - 1);
	if (seg_size == SEG_1T) {
		va |= TLBIE_RB_1TB;
	}

	if (actual == PAGE_16M) {
		/*
		 * 16M pages.
		 */
		if (base == PAGE_4K) {
			va |= TLBIE_RB_L |
				(TLBIE_RB_AP_4K_16M << TLBIE_RB_AP_SHIFT);
		} else {
			va |= (TLBIE_RB_LP_16M_16M << TLBIE_RB_LP_SHIFT);
			/*
			 * The AVAL bits represent those bits that end
			 * getting overlapped by LP. Unneeded bits must
			 * be ignored by the CPU. See p928.
			 */
			va |= (vpn & 0xfe);
		}
	}

	asm volatile (PPC_TLBIE(%1, %0) : : "r"(va), "r"(0));
	eieio();
	tlbsync();
	ptesync();
}


static
int
mmu_unmap_vpn(vpn_t vpn,
	      seg_size_t seg_size,
	      page_size_t base,
	      page_size_t actual)
{
	length_t base_size;
	length_t actual_size;

	BUG_ON(base != PAGE_4K && base != PAGE_16M,
		       "page_size_t %u not supported", base);
	BUG_ON(actual != PAGE_4K && actual != PAGE_16M,
		       "page_size_t %u not supported", actual);

	if (base == PAGE_16M) {
		base_size = PAGE_SIZE_16M;
	} else {
		base_size = PAGE_SIZE;
	}

	if (actual == PAGE_16M) {
		actual_size = PAGE_SIZE_16M;
	} else {
		actual_size = PAGE_SIZE;
	}

	BUG_ON(actual_size < base_size, "actual size cannot be less than base");

	while (actual_size != 0) {
		int i;
		uint64_t hash = vpn_hash(vpn, seg_size, base);
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
					  avpn_encode(vpn, base, actual))) {
				break;
			}
		}

		/*
		 * LOG("VPN 0x%x -> hash 0x%x -> pteg 0x%x "
		 *        "(i = %u v = 0x%x r = 0x%x) = unmap",
		 *        vpn, hash, pteg, i, be64_to_cpu(pte->v),
		 *        be64_to_cpu(pte->r));
		 */

		if (i == PTES_PER_GROUP) {
			return -1;
		}


		pte->v = be64_to_cpu(0);
		ptesync();
		tlbie(vpn, seg_size, base, actual);

		/*
		 * Update the next PTE that is part of the large PTE group.
		 */
		actual_size -= base_size;
		vpn += (base_size >> VPN_SHIFT);
	}

	return 0;
}


void
mmu_unmap(ea_t ea,
	  page_size_t actual)
{
	int ret;
	length_t actual_size;

	BUG_ON(actual != PAGE_4K && actual != PAGE_16M,
	       "page_size_t %u not supported", actual);

	if (actual == PAGE_16M) {
		actual_size = PAGE_SIZE_16M;
	} else {
		actual_size = PAGE_SIZE;
	}

	BUG_ON((ea & ~(actual_size - 1)) != ea,
	       "ea 0x%x not aligned to 0x%x", ea, actual_size);

	ret = mmu_unmap_vpn(ea_2_vpn(ea),
			    ea_2_seg_size(ea),
			    ea_2_base_size(ea),
			    actual);
	BUG_ON(ret != 0, "EA 0x%x not mapped", ea);
}


static int
mmu_map_vpn(vpn_t vpn,
	    ra_t ra,
	    prot_t pp,
	    seg_size_t seg_size,
	    page_size_t base,
	    page_size_t actual)
{
	length_t base_size;
	length_t actual_size;

	uint64_t rflags = pp | PTE_R_M;
	uint64_t vflags = PTE_R_M | PTE_V_VALID |
		((actual != PAGE_4K) ? PTE_V_LARGE : 0);

	if (seg_size == SEG_1T) {
		vflags |= PTE_V_1TB_SEG;
	}

	BUG_ON(base != PAGE_4K && base != PAGE_16M,
	       "page_size_t %u not supported", base);
	BUG_ON(actual != PAGE_4K && actual != PAGE_16M,
		       "page_size_t %u not supported", actual);

	if (base == PAGE_16M) {
		base_size = PAGE_SIZE_16M;
	} else {
		base_size = PAGE_SIZE;
	}

	if (actual == PAGE_16M) {
		actual_size = PAGE_SIZE_16M;
	} else {
		actual_size = PAGE_SIZE;
	}

	BUG_ON(actual_size < base_size,
	       "actual size 0x%x cannot be less than base 0x%x",
               actual_size, base_size);
	BUG_ON((ra & ~(actual_size - 1)) != ra,
	       "RA 0x%x not aligned to 0x%x", ra, actual_size);

	/*
	 * In a loop, with ea incrementing by base_size, to accomodate
	 * large pages in a mixed page environment.
	 */
	while (actual_size != 0) {
		int i;
		uint64_t v;
		uint64_t r;
		uint64_t hash = vpn_hash(vpn, seg_size, base);
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
		 * LOG("VPN 0x%x -> hash 0x%x -> pteg 0x%x (i = %u) = RA 0x%x",
		 *         vpn, hash, pteg, i, ra);
		 */
		if (i == PTES_PER_GROUP) {
			return -1;
		}

		v = avpn_encode(vpn, base, actual) | vflags;
		r = rpn_encode(ra, base, actual) | rflags;

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
		actual_size -= base_size;
		vpn += (base_size >> VPN_SHIFT);
	}

	return 0;
}


void
mmu_map(ea_t ea,
	ra_t ra,
	prot_t pp,
	page_size_t actual)
{
	int ret;
	length_t actual_size;

	BUG_ON(actual != PAGE_4K && actual != PAGE_16M,
		       "page_size_t %u not supported", actual);

	if (actual == PAGE_16M) {
		actual_size = PAGE_SIZE_16M;
	} else {
		actual_size = PAGE_SIZE;
	}

	BUG_ON((ea & ~(actual_size - 1)) != ea,
	       "EA 0x%x not aligned to 0x%x", ea, actual_size);

	ret = mmu_map_vpn(ea_2_vpn(ea), ra, pp,
			  ea_2_seg_size(ea),
			  ea_2_base_size(ea),
			  actual);
	BUG_ON(ret != 0, "PTEG spill for EA 0x%x", ea);
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

	BUG_ON((ea_end - ea_start) % size != 0, "range [0x%lx, 0x%lx) not 0x%x aligned",
	       ea_start, ea_end, size);

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
	htab = mem_memalign(HTAB_ALIGN, htab_size);
	LOG("HTAB (%u ptegs, mask 0x%x, size 0x%x) @ %p",
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

	tlbia();

	/*
	 * Only map the bare minimum.
	 *
	 * Loaded at 0x00000000200XXXXX.
	 * We run at 0x80000000200XXXXX.
	 *
	 * MMU-mode vectors at EA 0xc000000000004000.
	 *
	 * Need these mappings in order to successfully
	 * take exceptions with MMU on (i.e. with AIL).
	 *
	 * (The rest gets faulted in by exc.c).
	 */
	mmu_map_range(((ea_t) htab) & PAGE_MASK_16M,
		      ALIGN_UP(((ea_t) htab) + htab_size, MB(16)),
		      htab_ra & PAGE_MASK_16M,
		      PP_RWXX,
		      PAGE_16M);
	mmu_map_range(((ea_t) &_start) & PAGE_MASK_16M,
		      ALIGN_UP((ea_t) &_end, MB(16)),
		      ptr_2_ra(&_start) & PAGE_MASK_16M,
		      PP_RWXX,
		      PAGE_16M);
	mmu_map_range(AIL_VECTORS,
		      AIL_ASPACE_END,
		      0,
		      PP_RWXX,
		      PAGE_4K);

	LOG("enabling mmu");
	mmu_enable();
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
