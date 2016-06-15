/*
 * Guest state.
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

#include <guest.h>
#include <log.h>
#include <mem.h>
#include <mmu.h>
#include <layout.h>
#include <string.h>
#include <rom.h>

guest_t *guest;

#define R(x) (&frame->r0)[(x)]


static void
guest_update_msr(uint32_t new)
{
	bool was = guest_is_mmu_off();
	guest->msr = new;

	if (was != guest_is_mmu_off()) {
		mmu_guest_update(!guest_is_mmu_off());
	}
}


err_t
guest_init(length_t ram_size)
{
	unsigned i;
	guest = mem_malloc(sizeof(guest_t));
	if (guest == NULL) {
		return ERR_NO_MEM;
	}

	BUG_ON(ram_size < MB(32), "too little RAM");
	BUG_ON(ram_size % MB(16) != 0, "RAM size % 16MB != 0");
	memset(guest, 0, sizeof(guest_t));

//	guest->pvr = 0x00010001; /* 601  - YES!   */
//	guest->pvr = 0x00030001; /* 603  - YES!   */
	guest->pvr = 0x00040103; /* 604  - YES!   */
//	guest->pvr = 0x00090204; /* 604e - YES!   */
//	guest->pvr = 0x00080200; /* G3   - YES!!! */
//	guest->pvr = 0x000c0200; /* G4   - NOPE.  */
	guest->msr = 0;
	guest->ram_size = ram_size;
	guest->ram = mem_memalign(MB(16), guest->ram_size);

	LOG("HV pointer to GRAM %p", guest->ram);
	LOG("Guest RAM at RA 0x%lx", ptr_2_ra(guest->ram));

	{
		int m;
		int megs = guest->ram_size  / MB(1);
		for (m = 0; m < megs; m++) {
			LOG("clearing mb %u", m);
                        memset(guest->ram + MB(m), 0, MB(1));
		}
	}

	for (i = 0; i < ARRAY_LEN(guest->sr); i++) {
		guest->sr[i] = (i << SR_VSID_SHIFT);
	}

	/*
	 * The GRM mapping (uses 16M pages).
	 */
	mmu_guest_update(false);
	mmu_map_range(LAYOUT_VM_START,
		      LAYOUT_VM_START + guest->ram_size,
		      ptr_2_ra(guest->ram),
		      PP_RWRW,
		      PAGE_16M);

	/*
	 * Our initial state isn't "real mode" per se. We
	 * initialize the shadow MMU and have reasonable SRs,
	 * but don't bother with guest HTAB.
	 */
	guest_update_msr(MSR_IR | MSR_DR);
	mmu_map_range(LAYOUT_VM_START,
		      LAYOUT_VM_START + guest->ram_size,
		      ptr_2_ra(guest->ram),
		      PP_RWRW,
		      PAGE_4K);

	return ERR_NONE;
}

err_t
guest_exc_try(eframe_t *frame)
{
	err_t err = ERR_UNSUPPORTED;

	if (frame->vec == EXC_DSEG) {
		if (get_DAR() >= LAYOUT_VM_END) {
			return ERR_UNSUPPORTED;
		}

		err =  mmu_guest_fault_seg(get_DAR());
	} else if (frame->vec == EXC_ISEG) {
		if (frame->hsrr0 >= LAYOUT_VM_END) {
			return ERR_UNSUPPORTED;
		}

		err = mmu_guest_fault_seg(frame->hsrr0);
	}

	if (err == ERR_NONE) {
		return ERR_NONE;
	}

	if ((frame->hsrr1 & MSR_SF) != 0) {
		return ERR_UNSUPPORTED;
	}

	BUG_ON((frame->hsrr1 & MSR_PR) == 0, "32-bit non-PR");

	if (frame->vec == EXC_PROG ||
	    frame->vec == EXC_HEA) {
#define PPC_INST_MTSPR_MASK             0xfc0007fe
#define PPC_INST_MFSPR_MASK             0xfc0007fe
#define PPC_INST_MTMSR_MASK             0xfc0007fe
#define PPC_INST_MFMSR_MASK             0xfc0007fe
#define PPC_INST_MFSR_MASK              0xfc0007fe
#define PPC_INST_RFI_MASK               0xfc0007fe

#define PPC_INST_MTSPR                  0x7c0003a6
#define PPC_INST_MFSPR                  0x7c0002a6
#define PPC_INST_MTMSR                  0x7c000124
#define PPC_INST_MFMSR                  0x7c0000a6
#define PPC_INST_MFSR                   0x7c0004a6
#define PPC_INST_RFI                    0x4c000064
		uint32_t *i = (uint32_t *) frame->hsrr0;

		if ((*i & PPC_INST_MFSR_MASK) == PPC_INST_MFSR) {
			int reg = MASK_OFF(*i, 25, 21);
			int sr = MASK_OFF(*i, 19, 16);
			R(reg) = guest->sr[sr];
			frame->hsrr0 += 4;
			return ERR_NONE;
		} else if ((*i & PPC_INST_MFMSR_MASK) == PPC_INST_MFMSR) {
			int reg = MASK_OFF(*i, 25, 21);
			R(reg) = guest->msr;
			frame->hsrr0 += 4;
			return ERR_NONE;
		} else if ((*i & PPC_INST_MTMSR_MASK) == PPC_INST_MTMSR) {
			int reg = MASK_OFF(*i, 25, 21);

			guest_update_msr(R(reg));
			frame->hsrr0 += 4;
			return ERR_NONE;
		} else if ((*i & PPC_INST_MTSPR_MASK) == PPC_INST_MTSPR) {
			int reg = MASK_OFF(*i, 25, 21);
			int spr = MASK_OFF(*i, 20, 11);
			spr = ((spr & 0x1f) << 5) | ((spr & 0x3e0) >> 5);
			switch (spr) {
			case SPRN_SRR0:
				guest->srr0 = R(reg);
				break;
			case SPRN_SRR1:
				guest->srr1 = R(reg);
				break;
			case SPRN_IBAT0U:
			case SPRN_IBAT0L:
			case SPRN_IBAT1U:
			case SPRN_IBAT1L:
			case SPRN_IBAT2U:
			case SPRN_IBAT2L:
			case SPRN_IBAT3U:
			case SPRN_IBAT3L:
			case SPRN_DBAT0U:
			case SPRN_DBAT0L:
			case SPRN_DBAT1U:
			case SPRN_DBAT1L:
			case SPRN_DBAT2U:
			case SPRN_DBAT2L:
			case SPRN_DBAT3U:
			case SPRN_DBAT3L: {
				uint32_t *batp = guest->ibat;
				batp[spr - SPRN_IBAT0U] = R(reg);
				ERROR("BAT%u = 0x%x (MMU off? %u)",
				      spr - SPRN_IBAT0U,
				      R(reg), guest_is_mmu_off());
				break;
			}
			case SPRN_SPRG0:
			case SPRN_SPRG1:
			case SPRN_SPRG2:
			case SPRN_SPRG3: {
				uint32_t *sprg = guest->sprg;
				sprg[spr - SPRN_SPRG0] = R(reg);
				ERROR("SPRG%u = 0x%x (MMU off? %u)",
				      spr - SPRN_SPRG0,
				      R(reg), guest_is_mmu_off());
				break;
			}
			case SPRN_SDR1:
				guest->sdr1 = R(reg);
				ERROR("SDR1 = 0x%x (MMU off? %u)", R(reg),
				      guest_is_mmu_off());
				break;
			default:
				FATAL("0x%x: unhandled MTSPR %u, r%u",
				      frame->hsrr0, spr, reg);
			}

			frame->hsrr0 += 4;
			return ERR_NONE;
		} else if ((*i & PPC_INST_MFSPR_MASK) == PPC_INST_MFSPR) {
			int reg = MASK_OFF(*i, 25, 21);
			int spr = MASK_OFF(*i, 20, 11);
			spr = ((spr & 0x1f) << 5) | ((spr & 0x3e0) >> 5);
			switch (spr) {
			case SPRN_SRR0:
				R(reg) = guest->srr0;
				break;
			case SPRN_SRR1:
				R(reg) = guest->srr1;
				break;
			case SPRN_PVR:
				R(reg) = guest->pvr;
				break;
			case SPRN_IBAT0U:
			case SPRN_IBAT0L:
			case SPRN_IBAT1U:
			case SPRN_IBAT1L:
			case SPRN_IBAT2U:
			case SPRN_IBAT2L:
			case SPRN_IBAT3U:
			case SPRN_IBAT3L:
			case SPRN_DBAT0U:
			case SPRN_DBAT0L:
			case SPRN_DBAT1U:
			case SPRN_DBAT1L:
			case SPRN_DBAT2U:
			case SPRN_DBAT2L:
			case SPRN_DBAT3U:
			case SPRN_DBAT3L: {
				uint32_t *batp = guest->ibat;
				R(reg) = batp[spr - SPRN_IBAT0U];
				break;
			}
			case SPRN_SPRG0:
			case SPRN_SPRG1:
			case SPRN_SPRG2:
			case SPRN_SPRG3: {
				uint32_t *sprg = guest->sprg;
				R(reg) = sprg[spr - SPRN_SPRG0];
				break;
			}
			case SPRN_SDR1:
				R(reg) = guest->sdr1;
				break;
			default:
				FATAL("0x%x: unhandled MFSPR r%u, %u",
				      frame->hsrr0, reg, spr);
			}

			frame->hsrr0 += 4;
			return ERR_NONE;
		} else if ((*i & PPC_INST_RFI_MASK) == PPC_INST_RFI) {
			guest_update_msr(guest->srr1);
			frame->hsrr0 = guest->srr0;
			return ERR_NONE;
		}

		ERROR("0x%x: unhandled insn 0x%x", frame->hsrr0, *i);
	} else if (frame->vec == EXC_SC) {
		mtmsrd(MSR_RI, 1);
		err = rom_call(frame);
		mtmsrd(0, 1);
		return err;
	}

	WARN("FIXME: inject exception into guest");
	return ERR_UNSUPPORTED;
}
