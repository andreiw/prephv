/*
 * OpenPower architecture definitions.
 * Most of this code is derived from the Linux Kernel.
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

#ifndef PPC_REGS_H
#define PPC_REGS_H

#ifdef __ASSEMBLY__
#define __MASK(X) (1 << (X))
#else
#define __MASK(X) (1UL << (X))
#endif

#define MSR_SF_LG       63              /* Enable 64 bit mode */
#define MSR_ISF_LG      61              /* Interrupt 64b mode valid on 630 */
#define MSR_HV_LG       60              /* Hypervisor state */
#define MSR_VEC_LG      25              /* Enable AltiVec */
#define MSR_POW_LG      18              /* Enable Power Management */
#define MSR_WE_LG       18              /* Wait State Enable */
#define MSR_TGPR_LG     17              /* TLB Update registers in use */
#define MSR_CE_LG       17              /* Critical Interrupt Enable */
#define MSR_ILE_LG      16              /* Interrupt Little Endian */
#define MSR_EE_LG       15              /* External Interrupt Enable */
#define MSR_PR_LG       14              /* Problem State / Privilege Level */
#define MSR_FP_LG       13              /* Floating Point enable */
#define MSR_ME_LG       12              /* Machine Check Enable */
#define MSR_FE0_LG      11              /* Floating Exception mode 0 */
#define MSR_SE_LG       10              /* Single Step */
#define MSR_BE_LG       9               /* Branch Trace */
#define MSR_DE_LG       9               /* Debug Exception Enable */
#define MSR_FE1_LG      8               /* Floating Exception mode 1 */
#define MSR_IP_LG       6               /* Exception prefix 0x000/0xFFF */
#define MSR_IR_LG       5               /* Instruction Relocate */
#define MSR_DR_LG       4               /* Data Relocate */
#define MSR_PE_LG       3               /* Protection Enable */
#define MSR_PX_LG       2               /* Protection Exclusive Mode */
#define MSR_PMM_LG      2               /* Performance monitor */
#define MSR_RI_LG       1               /* Recoverable Exception */
#define MSR_LE_LG       0               /* Little Endian */

#define MSR_SF          __MASK(MSR_SF_LG)       /* Enable 64 bit mode */
#define MSR_ISF         __MASK(MSR_ISF_LG)      /* Interrupt 64b mode valid on 630 */
#define MSR_HV          __MASK(MSR_HV_LG)       /* Hypervisor state */
#define MSR_VEC         __MASK(MSR_VEC_LG)      /* Enable AltiVec */
#define MSR_POW         __MASK(MSR_POW_LG)      /* Enable Power Management */
#define MSR_WE          __MASK(MSR_WE_LG)       /* Wait State Enable */
#define MSR_TGPR        __MASK(MSR_TGPR_LG)     /* TLB Update registers in use */
#define MSR_CE          __MASK(MSR_CE_LG)       /* Critical Interrupt Enable */
#define MSR_ILE         __MASK(MSR_ILE_LG)      /* Interrupt Little Endian */
#define MSR_EE          __MASK(MSR_EE_LG)       /* External Interrupt Enable */
#define MSR_PR          __MASK(MSR_PR_LG)       /* Problem State / Privilege Level */
#define MSR_FP          __MASK(MSR_FP_LG)       /* Floating Point enable */
#define MSR_ME          __MASK(MSR_ME_LG)       /* Machine Check Enable */
#define MSR_FE0         __MASK(MSR_FE0_LG)      /* Floating Exception mode 0 */
#define MSR_SE          __MASK(MSR_SE_LG)       /* Single Step */
#define MSR_BE          __MASK(MSR_BE_LG)       /* Branch Trace */
#define MSR_DE          __MASK(MSR_DE_LG)       /* Debug Exception Enable */
#define MSR_FE1         __MASK(MSR_FE1_LG)      /* Floating Exception mode 1 */
#define MSR_IP          __MASK(MSR_IP_LG)       /* Exception prefix 0x000/0xFFF */
#define MSR_IR          __MASK(MSR_IR_LG)       /* Instruction Relocate */
#define MSR_DR          __MASK(MSR_DR_LG)       /* Data Relocate */
#define MSR_PE          __MASK(MSR_PE_LG)       /* Protection Enable */
#define MSR_PX          __MASK(MSR_PX_LG)       /* Protection Exclusive Mode */
#define MSR_PMM         __MASK(MSR_PMM_LG)      /* Performance monitor */
#define MSR_RI          __MASK(MSR_RI_LG)       /* Recoverable Exception */
#define MSR_LE          __MASK(MSR_LE_LG)       /* Little Endian */

#define SPRN_HID0       0x3F0           /* Hardware Implementation Register 0 */
#define HID0_HILE       __MASK(63 - 19) /* Hypervisor interrupt LE on Power8 */
#define SPRN_LPCR       0x13E           /* LPAR Control Register */
#define SPRN_HRMOR      0x139           /* Real mode offset register */
#define SPRN_HSRR0      0x13A           /* Save/Restore Register 0 */
#define SPRN_HSRR1      0x13B           /* Save/Restore Register 1 */
#define SPRN_HSPRG0     0x130           /* Hypervisor Scratch 0 */
#define SPRN_HSPRG1     0x131           /* Hypervisor Scratch 1 */

#define EXC_START       0x0
#define EXC_RESET       0x100
#define EXC_MC          0x200
#define EXC_DSI         0x300
#define EXC_DSEG        0x380
#define EXC_ISI         0x400
#define EXC_ISEG        0x480
#define EXC_EXT         0x500
#define EXC_ALIGN       0x600
#define EXC_PROG        0x700
#define EXC_FPU         0x800
#define EXC_DEC         0x900
#define EXC_HDEC        0x980
#define EXC_DOOR        0xA00
#define EXC_RESV        0xB00
#define EXC_SC          0xC00
#define EXC_TRACE       0xD00
#define EXC_HDSI        0xE00
#define EXC_HISI        0xE20
#define EXC_HEA         0xE40
#define EXC_HMAINT      0xE60
#define EXC_HDOOR       0xE80
#define EXC_RESV1       0xEA0
#define EXC_RESV2       0xEC0
#define EXC_IMPL        0xEE0
#define EXC_PMU         0xF00
#define EXC_VECUN       0xF20
#define EXC_VSX         0xF40
#define EXC_FAC         0xF60
#define EXC_HFAC        0xF80
#define EXC_TABLE_END   0xFFF

#endif /* PPC_REGS_H */
