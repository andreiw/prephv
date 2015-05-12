#define FIXUP_ENDIAN							\
	tdi   0, 0, 0x48; /* Reverse endian of b . + 8          */	\
	b     $+36;       /* Skip trampoline if endian is good  */	\
	.long 0x05009f42; /* bcl 20,31,$+4                      */	\
	.long 0xa602487d; /* mflr r10                           */	\
	.long 0x1c004a39; /* addi r10,r10,28                    */	\
	.long 0xa600607d; /* mfmsr r11                          */	\
	.long 0x01006b69; /* xori r11,r11,1                     */	\
	.long 0xa6035a7d; /* mtsrr0 r10                         */	\
	.long 0xa6037b7d; /* mtsrr1 r11                         */	\
	.long 0x2400004c  /* rfid                               */

#define LOAD_IMM64(r, e)                        \
	lis     r,(e)@highest;                  \
	ori     r,r,(e)@higher;                 \
	rldicr  r,r, 32, 31;                    \
	oris    r,r, (e)@h;                     \
	ori     r,r, (e)@l;

#define LOAD_ADDR(r, n)                         \
	ld      r,n@got(%r2)

#define STACKFRAMESIZE 256
#define __STK_REG(i)   (112 + ((i)-14)*8)
#define STK_REG(i)     __STK_REG(__REG_##i)
#define STK_GOT		24
#define STK_LR		16
#define __STK_PARAM(i)	(32 + ((i)-3)*8)

#define _GLOBAL(name) \
	.section ".text"; \
	.align 2 ; \
	.type name,@function; \
	.globl name; \
name:

#define _GLOBAL_TOC(name) \
	.section ".text"; \
	.align 2 ; \
	.type name,@function; \
	.globl name; \
name: \
0:	addis r2,r12,(.TOC.-0b)@ha; \
	addi r2,r2,(.TOC.-0b)@l;    \
	.localentry name,.-name

#define __MASK(X)       (1<<(X))

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

#define SPRN_HSRR0      0x13A   /* Save/Restore Register 0 */
#define SPRN_HSRR1      0x13B   /* Save/Restore Register 1 */
