/*
 * Nifty stuff for writing asm bits.
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

#ifndef ASM_UTILS_H
#define ASM_UTILS_H

#define r0  %r0
#define r1  %r1
#define r2  %r2
#define r3  %r3
#define r4  %r4
#define r5  %r5
#define r6  %r6
#define r7  %r7
#define r8  %r8
#define r9  %r9
#define r10 %r10
#define r11 %r11
#define r12 %r12
#define r13 %r13
#define r14 %r14
#define r15 %r15
#define r16 %r16
#define r17 %r17
#define r18 %r18
#define r19 %r19
#define r20 %r20
#define r21 %r21
#define r22 %r22
#define r23 %r23
#define r24 %r24
#define r25 %r25
#define r26 %r26
#define r27 %r27
#define r28 %r28
#define r29 %r29
#define r30 %r30
#define r31 %r31

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
	ld      r,n@got(r2)

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

#endif /* ASM_UTILS_H */
