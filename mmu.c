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
#include <console.h>
#include <linkage.h>
#include <endian.h>
#include <string.h>
#include <kpcr.h>
#include <ppc.h>
#include <mmu.h>

/*
 * in order to fit the 78 bit va in a 64 bit variable we shift the va by
 * 12 bits. This enable us to address upto 76 bit va.
 * For hpt hash from a va we can ignore the page size bits of va and for
 * hpte encoding we ignore up to 23 bits of va. So ignoring lower 12 bits ensure
 * we work in all cases including 4k page size.
 */
#define VPN_SHIFT 12
typedef uint64_t vpn_t;

/*
 * ESID(0) => VSID(0).
 */
#define EA_TO_VPN(ea) (ea >> VPN_SHIFT)

typedef struct {
	__be64 v;
	__be64 r;
} pte_t;

static void *htab;
static uint64_t ptegs;
static uint64_t htab_size;
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

	/*
	 * We don't do anything too crazy here, just set up 1
	 * segment where ESID (0) == VSID (0). Our ISI/DSI
	 * handlers will simply map the VA to RA 1:1, as long as
	 * VA is not beyond &_end.
	 */

	asm volatile("slbia" ::: "memory");
	isync();

	/*
	 * 1TB segment at EA=0 => VA=0. We use slot = 1,
	 * since slot 0 has special handling (not
	 * invalidated with tlbia).
	 */
	esid = slb_make_esid(0, 1);
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
vpn_hash(vpn_t vpn)
{
	vpn_t mask, hash, vsid;

	mask = (1ul << (SID_SHIFT_1T - VPN_SHIFT)) - 1;
	vsid = vpn >> (SID_SHIFT_1T - VPN_SHIFT);
	hash = vsid ^ (vsid << 25) ^
		((vpn & mask) >> (PAGE_SHIFT - VPN_SHIFT));

	return hash & 0x7fffffffffUL;
}


static inline uint64_t
rpn_encode(ra_t ra)
{
	/*
	 * 4K pages are easy.
	 */
	return ra & PTE_R_RPN;
}


static inline uint64_t
avpn_encode(vpn_t vpn)
{
	uint64_t v;
	/*
	 * The AVA field omits the low-order 23 bits of the 78 bits VA.
	 * These bits are not needed in the PTE, because the
	 * low-order b of these bits are part of the byte offset
	 * into the virtual page and, if b < 23, the high-order
	 * 23-b of these bits are always used in selecting the
	 * PTEGs to be searched
	 */
	v = vpn >> (23 - VPN_SHIFT);
	v <<= PTE_V_AVPN_SHIFT;
	return v;
}


static inline void
tlbie(vpn_t vpn)
{
	uint64_t va = vpn << VPN_SHIFT;
	va &= ~((1ul << (64 - 52)) - 1);
	va |= TLBIE_RB_1TB;
	asm volatile (PPC_TLBIE(%1, %0) : : "r"(va), "r"(0));
	eieio();
	tlbsync();
	ptesync();
}


int
mmu_unmap(ea_t ea)
{
	int i;
	vpn_t vpn = EA_TO_VPN(ea);
	uint64_t hash = vpn_hash(vpn);
	uint64_t pteg = ((hash & htab_hash_mask) * PTES_PER_GROUP);
	pte_t *pte = ((pte_t *) htab) + pteg;

	/*
	 * We don't do secondary PTEGs and we're not SMP safe:
	 * real OSes use one of the software bits inside the V
	 * part of pte_t as a spinlock.
	 */
	printk("EA 0x%x -> hash 0x%x -> pteg 0x%x = unmap\n",
	       ea, hash, pteg);
	for (i = 0; i < PTES_PER_GROUP; i++, pte++) {
		if ((be64_to_cpu(pte->v) & PTE_V_VALID) == 0) {
			/*

			 * Not used, go to next slot.
			 */
			continue;
		}

		if (PTE_V_COMPARE(be64_to_cpu(pte->v),
				  avpn_encode(vpn)) == 0) {
			break;
		}
	}

	if (i == PTES_PER_GROUP) {
		printk("EA 0x%x not mapped\n", ea);
		return -1;
	}

	pte->v = be64_to_cpu(0);
	ptesync();
	tlbie(vpn);

	return 0;
}


int
mmu_map(ea_t ea, ra_t ra, prot_t pp)
{
	int i;
	uint64_t v;
	uint64_t r;
	uint64_t rflags = pp | PTE_R_M;
	uint64_t vflags = PTE_V_1TB_SEG | PTE_R_M | PTE_V_VALID;
	uint64_t hash = vpn_hash(EA_TO_VPN(ea));
	uint64_t pteg = ((hash & htab_hash_mask) * PTES_PER_GROUP);
	pte_t *pte = ((pte_t *) htab) + pteg;

	/*
	 * We don't do secondary PTEGs and we're not SMP safe:
	 * real OSes use one of the software bits inside the V
	 * part of pte_t as a spinlock.
	 */
	printk("EA 0x%x -> hash 0x%x -> pteg 0x%x = RA 0x%x\n",
	       ea, hash, pteg, ra);
	for (i = 0; i < PTES_PER_GROUP; i++, pte++) {
		if ((be64_to_cpu(pte->v) & PTE_V_VALID) != 0) {
			/*
			 * Busy, go to next slot.
			 */
			continue;
		}

		break;
	}

	if (i == PTES_PER_GROUP) {
		printk("Couldn't map EA 0x%x\n", ea);
		return -1;
	}

	v = avpn_encode(EA_TO_VPN(ea)) | vflags;
	r = rpn_encode(ra) | rflags;

	/*
	 * Unnecessary here (there were no other mappings),
	 * but if we were recycling a slot, we'd:
	 * - pte_t.v.PTE_V_VALID = 0
	 * - ptesync
	 * - tlbie(recycled VPN)
		 */
	tlbie(EA_TO_VPN(ea));

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

	return 0;
}


int
mmu_map_range(ea_t ea_start,
	      ea_t ea_end,
	      ra_t ra_start,
	      prot_t prot)
{
	int ret;
	ea_t addr = ea_start;
	ra_t ra = ra_start;

	for (; addr < ea_end; addr += PAGE_SIZE, ra += PAGE_SIZE) {
		ret = mmu_map(addr, ra, prot);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

int
mmu_init(uint64_t ram_size)
{
	int ret;
	ptegs = pteg_count(ram_size);
	htab_size = ptegs * PTEG_SIZE;
	htab_hash_mask = ptegs - 1;

	/*
	 * This requires Relaxed Page Table Alignment.
	 *
	 * See 5.7.7.4 PowerISA v2.07 p904.
	 */
	htab = PALIGN_UP(&_end, HTAB_ALIGN);
	printk("HTAB (%u ptegs, mask 0x%x, size 0x%x) @ %p\n",
	       ptegs, htab_hash_mask, htab_size, htab);

	memset(htab, 0, htab_size);

	/*
	 * The HTABSIZE field in SDR1 contains an integer giving
	 * the number of bits (in addition to the minimum of 11
	 * bits) from the hash that are used in the Page Table
	 * index. This number must not exceed 28.
	 */
	set_SDR1((uint64_t) htab + __ilog2(ptegs) - 11);

	slb_init();

	/*
	 * Only map the bare minimum 1:1.
	 */
	ret = mmu_map_range((uint64_t) htab,
			    ((uint64_t) htab) + htab_size,
			    (uint64_t) htab,
			    PP_RWXX);
	if (ret != 0) {
		printk("Couldn't map HTAB\n");
		return ret;
	}
	ret = mmu_map_range((uint64_t) &_start,
			    (uint64_t) &_end,
			    (uint64_t) &_start,
			    PP_RWXX);
	if (ret != 0) {
		printk("Couldn't map kernel binary\n");
		return ret;
	}

	tlbsync();
	return 0;
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


int
mmu_enabled(void)
{
	return (mfmsr() & (MSR_IR | MSR_DR)) == (MSR_IR | MSR_DR);
}
