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

#define STACKFRAMESIZE 256
#define __STK_REG(i)   (112 + ((i)-14)*8)
#define STK_REG(i)     __STK_REG(__REG_##i)
#define STK_GOT		24
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
