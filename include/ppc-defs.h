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

/*
 * Power Architecture 64-Bit ELF V2 ABI: The minimum stack frame size shall be
 * 32 bytes. A minimum stack frame consists of the first 4 double-
 * words (back-chain doubleword, CR save word and reserved word,
 * LR save doubleword, and TOC pointer doubleword), with padding to meet
 * the 16-byte alignment requirement.
 */
#define STACKFRAMEMIN  32
#define STACKFRAMESIZE 256
/*
 * ELF v2 ABI, p46, 2.2.2.4 Protected Zone.
 *
 * The 288 bytes below the stack pointer are available as volatile program
 * storage that is not preserved across function calls. Interrupt handlers
 * and any other functions that might run without an explicit call must take care
 * to preserve a protected zone, also referred to as the red zone, of 512 bytes
 * that consists of:
 * - The 288-byte volatile program storage region that is used to hold saved
 *   registers and local variables
 * - An additional 224 bytes below the volatile program storage region that
 *   is set aside as a volatile system storage region for system functions
 *
 * If a function does not call other functions and does not need more stack
 * space than is available in the volatile program storage region (that is,
 * 288 bytes), it does not need to have a stack frame. The 224-byte volatile
 * system storage region is not available to compilers for allocation to
 * saved registers and local variables.
 */
#define ABI_STACK_PROTECTED_ZONE 512
#define __STK_REG(i)   (112 + ((i)-14)*8)
#define STK_REG(i)     __STK_REG(__REG_##i)
#define STK_GOT		24
#define STK_LR		16
#define STK_CR		8
#define __STK_PARAM(i)	(32 + ((i)-3)*8)

#ifdef __ASSEMBLY__
#define __MASK(X) (1 << (X))
#else
#define __MASK(X) (1UL << (X))
#endif

#define MSR_SF_LG       63              /* Enable 64 bit mode */
#define MSR_HV_LG       60              /* Hypervisor state */
#define MSR_SLE_LG      58              /* Split Little Endian */
#define MSR_TS1_LG      34              /* Transaction State 0 */
#define MSR_TS0_LG      33              /* Transaction State 1 */
#define MSR_TSA_LG      32              /* Transactional Memory Available */
#define MSR_VEC_LG      25              /* Enable AltiVec */
#define MSR_VSX_LG      23              /* Enable VSX */
#define MSR_EE_LG       15              /* External Interrupt Enable */
#define MSR_PR_LG       14              /* Problem State / Privilege Level */
#define MSR_FP_LG       13              /* Floating Point enable */
#define MSR_ME_LG       12              /* Machine Check Enable */
#define MSR_FE0_LG      11              /* Floating Exception mode 0 */
#define MSR_SE_LG       10              /* Single Step */
#define MSR_BE_LG       9               /* Branch Trace */
#define MSR_FE1_LG      8               /* Floating Exception mode 1 */
#define MSR_IR_LG       5               /* Instruction Relocate */
#define MSR_DR_LG       4               /* Data Relocate */
#define MSR_PMM_LG      2               /* Performance monitor */
#define MSR_RI_LG       1               /* Recoverable Exception */
#define MSR_LE_LG       0               /* Little Endian */

#define MSR_SF          __MASK(MSR_SF_LG)       /* Enable 64 bit mode */
#define MSR_HV          __MASK(MSR_HV_LG)       /* Hypervisor state */
#define MSR_SLE         __MASK(MSR_SLE_LG)      /* Split Little Endian */
#define MSR_TS1         __MASK(MSR_TS1_LG)      /* Transaction State 0 */
#define MSR_TS0         __MASK(MSR_TS0_LG)      /* Transcation State 1 */
#define MSR_TSA         __MASK(MSR_TSA_LG)      /* Transactional Memory */
#define MSR_VEC         __MASK(MSR_VEC_LG)      /* Enable AltiVec */
#define MSR_VSX         __MASK(MSR_VSX_LG)      /* Enable VSX */
#define MSR_EE          __MASK(MSR_EE_LG)       /* External Interrupt Enable */
#define MSR_PR          __MASK(MSR_PR_LG)       /* Problem State / Privilege Level */
#define MSR_FP          __MASK(MSR_FP_LG)       /* Floating Point enable */
#define MSR_ME          __MASK(MSR_ME_LG)       /* Machine Check Enable */
#define MSR_FE0         __MASK(MSR_FE0_LG)      /* Floating Exception mode 0 */
#define MSR_SE          __MASK(MSR_SE_LG)       /* Single Step */
#define MSR_BE          __MASK(MSR_BE_LG)       /* Branch Trace */
#define MSR_FE1         __MASK(MSR_FE1_LG)      /* Floating Exception mode 1 */
#define MSR_IR          __MASK(MSR_IR_LG)       /* Instruction Relocate */
#define MSR_DR          __MASK(MSR_DR_LG)       /* Data Relocate */
#define MSR_PMM         __MASK(MSR_PMM_LG)      /* Performance monitor */
#define MSR_RI          __MASK(MSR_RI_LG)       /* Recoverable Exception */
#define MSR_LE          __MASK(MSR_LE_LG)       /* Little Endian */

#define SPRN_HID0       0x3F0           /* Hardware Implementation Register 0 */
#define HID0_HILE       __MASK(63 - 19) /* Hypervisor interrupt LE on Power8 */
#define SPRN_LPCR       0x13E           /* LPAR Control Register */
#define SPRN_HRMOR      0x139           /* Real mode offset register */
#define SPRN_SRR0       0x01A           /* Save/Restore Register 0 */
#define SPRN_SRR1       0x01B           /* Save/Restore Register 1 */
#define SPRN_HSRR0      0x13A           /* Save/Restore Register 0 */
#define SPRN_HSRR1      0x13B           /* Save/Restore Register 1 */
#define SPRN_HSPRG0     0x130           /* Hypervisor Scratch 0 */
#define SPRN_HSPRG1     0x131           /* Hypervisor Scratch 1 */
#define SPRN_HDEC       0x136           /* Hypervisor decrementer */
#define SPRN_DEC        0x16            /* Decrementer. */
#define SPRN_SDR1       0x019           /* HTAB base. */
#define SPRN_DAR        0x013           /* Data Adress Register. */
#define SPRN_DSISR      0x012           /* Data Storage Interrupt Status */
#define SPRN_SPRG0      0x110           /* Scratch 0 */
#define SPRN_SPRG1      0x111           /* Scratch 1 */
#define SPRN_SPRG2      0x112           /* Scratch 2 */
#define SPRN_SPRG3      0x113           /* Scratch 3 */
#define SPRN_PVR        0x11f           /* Processor Version Register */
#define SPRN_IBAT0U     0x210           /* IBAT0 upper */
#define SPRN_IBAT0L     0x211           /* IBAT0 lower */
#define SPRN_IBAT1U     0x212           /* IBAT1 upper */
#define SPRN_IBAT1L     0x213           /* IBAT1 lower */
#define SPRN_IBAT2U     0x214           /* IBAT2 upper */
#define SPRN_IBAT2L     0x215           /* IBAT2 lower */
#define SPRN_IBAT3U     0x216           /* IBAT3 upper */
#define SPRN_IBAT3L     0x217           /* IBAT3 lower */
#define SPRN_DBAT0U     0x218           /* DBAT0 upper */
#define SPRN_DBAT0L     0x219           /* DBAT0 lower */
#define SPRN_DBAT1U     0x21A           /* DBAT1 upper */
#define SPRN_DBAT1L     0x21B           /* DBAT1 lower */
#define SPRN_DBAT2U     0x21C           /* DBAT2 upper */
#define SPRN_DBAT2L     0x21D           /* DBAT2 lower */
#define SPRN_DBAT3U     0x21E           /* DBAT3 upper */
#define SPRN_DBAT3L     0x21F           /* DBAT3 lower */

#define DSISR_NOT_MAPPED_LG    (63 - 33)
#define DSISR_NOT_MAPPED       __MASK(DSISR_NOT_MAPPED_LG)
#define SRR1_ISI_NOT_MAPPED_LG (63 -33)
#define SRR1_ISI_NOT_MAPPED    __MASK(SRR1_ISI_NOT_MAPPED_LG)

#define LPCR_ILE_LG     (63 - 38)              /* Interrupt Little Endian */
#define LPCR_ILE        __MASK(LPCR_ILE_LG)
#define LPCR_AIL1_LG    (63 - 39)              /* Alternate Interrupt Location 1 */
#define LPCR_AIL1        __MASK(LPCR_AIL1_LG)
#define LPCR_AIL0_LG    (63 - 40)              /* Alternate Interrupt Location 0 */
#define LPCR_AIL0       __MASK(LPCR_AIL0_LG)
#define LPCR_LPES_LG    (63 - 60)              /* Logical Part. Env. Selector */
#define LPCR_LPES       __MASK(LPCR_LPES_LG)
#define LPCR_HDICE_LG   (63 - 63)              /* Hypervisor Decrementer Enable */
#define LPCR_HDICE      __MASK(LPCR_HDICE_LG)

/*
 * We can't really disable the regular decrementer, but we can set it
 * to the maximum value that doesn't cause an interrupt, which is
 * the value with MSB bit set to 0.
 */
#define DEC_DISABLE     0x7fffffff

/*
 * Exception offsets.
 */
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

/*
 * HV real mode access that bypasses HRMOR. We don't use HRMOR,
 * but this lets us link to 0x8000000020010000, even though
 * skiboot loads us to 0x20010000.
 *
 * Useful feature, for example, because Power ISA does not define BATs any more
 * and paging is a PITA to set up early in the game (and low addresses suck,
 * esp. for a hypervisor).
 */
#define HV_ASPACE           0x8000000000000000UL

/*
 * Alternative interrupt vector space, reserve 1M, which
 * is more than we'll ever need.
 *
 * Actual vectors starts at offset 0x4000.
 */
#define AIL_ASPACE_START    0xc000000000000000UL
#define AIL_VECTORS         0xc000000000004000UL
#define AIL_ASPACE_END      0xc000000000100000UL

/*
 * GNU tools somehow don't know about the 2-register tlbie.
 *
 * This is awful.
 */
#define PPC_INST_TLBIE      0x7c000264
#define ___PPC_RS(s)        (((s) & 0x1f) << 21)
#define ___PPC_RB(b)        (((b) & 0x1f) << 11)
#define PPC_TLBIE(lp,a)     SIFY(.long PPC_INST_TLBIE | \
                              ___PPC_RB(a) | ___PPC_RS(lp))
#define TLBIE_RB_1TB        (1 << 9)
#define TLBIE_RB_L          (1 << 0)
#define TLBIE_RB_AP_SHIFT   (7)
#define TLBIE_RB_AP_4K_16M  (0x4)
#define TLBIE_RB_LP_SHIFT   (19)
#define TLBIE_RB_LP_16M_16M (0)

/*
 * Tlbia emulation.
 */
#define TLBIEL_MAX_SETS         (1 << (51 - 40 + 1))
#define TLBIEL_SET_SHIFT        (63-51)
#define TLBIEL_IS_ALL_IN_SET    0x3
#define TLBIEL_IS_SHIFT         (64-53)

/*
 * Segmentation defines. The hypervisor uses 1T segments,
 * but for the guest we use 256MB since we need to emulate
 * 32-bit segmentation.
 */

/* 1T segments. */
#define SID_SHIFT_1T            40
#define ESID_MASK_1T            0xffffff0000000000UL

/* 256MB segments. */
#define SID_SHIFT_256MB         28
#define ESID_MASK_256MB         0xfffffffff0000000UL

/* Bits in the SLB ESID word */
#define SLB_ESID_V              (0x0000000008000000) /* valid */

/* Bits in the SLB VSID word */
#define SLB_VSID_SHIFT_1T       24
#define SLB_VSID_SHIFT_256MB    12
#define SLB_VSID_B_1T           (1UL << (63-1))
#define SLB_VSID_KS             (1UL << (63-52))
#define SLB_VSID_KP             (1UL << (63-53))
#define SLB_VSID_L              (1UL << (63-55))
#define SLB_VSID_LP_SHIFT       (63-58)
#define SLB_VSID_LP_16M         0
#define SLB_VSID_LP_4K          0

/* 32-bit segment register definitions */
#define SR_INDEX(ea)            (ea >> 28)
#define SR_T                    (1U << (31 - 0))
#define SR_KP                   (1U << (31 - 1))
#define SR_KS                   (1U << (31 - 2))
#define SR_VSID_MASK            0xFFFFFF
#define SR_VSID_SHIFT           0
/*
 * HTAB support.
 *
 * See 5.7.7.2 PowerISA v2.07 p901,
 *     5.7.7.4 PowerISA v2.07 p904.
 */
#define PTEG_SIZE      128
#define PTES_PER_GROUP 8
#define MIN_PTEGS  (1UL << 11)
#define MAX_PTEGS  (1UL << (11 + 28))
#define HTAB_ALIGN (MIN_PTEGS * PTEG_SIZE)
#define SDR1_MASK (~(HTAB_ALIGN - 1) |  0xF000000000000000UL)

/*
 * Base page size if 4K.
 */
#define PAGE_SHIFT       12
#define PAGE_SIZE        (1 << PAGE_SHIFT)
#define PAGE_MASK        (~(PAGE_SIZE - 1))
#define PAGE_SHIFT_16M   24
#define PAGE_SIZE_16M    (1 << PAGE_SHIFT_16M)
#define PAGE_MASK_16M    (~(PAGE_SIZE_16M - 1))

/*
 * PTE bits.
 */
#define PTE_V_1TB_SEG          (0x4000000000000000)
#define PTE_V_SSIZE_SHIFT      62
#define PTE_V_AVPN_SHIFT       7
#define PTE_V_AVPN             (0x3fffffffffffff80)
#define PTE_V_AVPN_VAL(x)      (((x) & PTE_V_AVPN) >> PTE_V_AVPN_SHIFT)
#define PTE_V_COMPARE(x,y)     (!(((x) ^ (y)) & PTE_V_AVPN))
#define PTE_V_LARGE            (0x0000000000000004)
#define PTE_V_SECONDARY        (0x0000000000000002)
#define PTE_V_VALID            (0x0000000000000001)

#define PTE_R_PP0              (0x8000000000000000)
#define PTE_R_TS               (0x4000000000000000)
#define PTE_R_KEY_HI           (0x3000000000000000)
#define PTE_R_RPN_SHIFT        12
#define PTE_R_RPN_4K           (0x0ffffffffffff000)
#define PTE_R_RPN_16M          (0x0fffffffff000000)
#define PTE_R_PP               (0x0000000000000003)
#define PTE_R_N                (0x0000000000000004)
#define PTE_R_G                (0x0000000000000008)
#define PTE_R_M                (0x0000000000000010)
#define PTE_R_I                (0x0000000000000020)
#define PTE_R_W                (0x0000000000000040)
#define PTE_R_WIMG             (0x0000000000000078)
#define PTE_R_LP_MASK          (0xff000)
#define PTE_R_LP_SHIFT         12
#define PTE_R_C                (0x0000000000000080)
#define PTE_R_R                (0x0000000000000100)
#define PTE_R_KEY_LO           (0x0000000000000e00)

/*
 * Power8 PTE_R LP flags for large pages.
 *
 * See P103 of the POWER8 Processor User's Manual for the Single-Chip Module
 */
#define PTE_R_LP_4K_4K         0
#define PTE_R_LP_4K_16M        0x38
#define PTE_R_LP_16M_16M       0

/* Protection values for PP (assumes Ks=0, Kp=1) */
#define PP_RWXX 0                 /* Supervisor read/write, User none */
#define PP_RWRX 1                 /* Supervisor read/write, User read */
#define PP_RWRW 2                 /* Supervisor read/write, User read/write */
#define PP_RXRX 3                 /* Supervisor read,       User read */
#define PP_RXXX (PTE_R_PP0 | 2)   /* Supervisor read, user none */

#endif /* PPC_REGS_H */
