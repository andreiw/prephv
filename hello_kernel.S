#include <asm-utils.h>

_GLOBAL(_start)
	FIXUP_ENDIAN
	lis     %r2,.TOC.@ha
	addi    %r2,%r2,.TOC.@l
	bl c_main
	ori  %r0,%r0,0
	attn

_GLOBAL(mambo_write)
#define SIM_WRITE_CONSOLE_CODE 0
	li	%r6, 0
	mr	%r5, %r4
	mr	%r4, %r3
	li	%r3, SIM_WRITE_CONSOLE_CODE
	.long	0x000eaeb0
	blr
