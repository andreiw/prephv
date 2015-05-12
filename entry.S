#include <asm-utils.h>

/*
* Entering into this code we make the following assumptions:
*      The MMU is off, processor in HV mode, primary CPU enters
*      with device-tree in r3. We also get OPAL base in r8 and
*      entry in r9 for debugging purposes.
*/
_GLOBAL(_start)
	FIXUP_ENDIAN

/*
 * Setup TOC for local calls.
 */
	LOAD_IMM64(%r2, .TOC.)

/*
 * Point thread pointer to our toy per-cpu structure
 * and set it up, storing OPAL base/entry and TOC away in it.
 */
	LOAD_IMM64(%r13, kpcr)
	std     %r8, (kpcr_opal_base-kpcr)(%r13)
	std     %r9, (kpcr_opal_entry-kpcr)(%r13)
	std     %r2, (kpcr_toc-kpcr)(%r13)
	bl      c_main
	ori     %r0,%r0,0
	attn

/*
 * r3 - buffer
 * r4 - len
 */
_GLOBAL(mambo_write)
#define SIM_WRITE_CONSOLE_CODE 0
	li	%r6, 0
	mr	%r5, %r4
	mr	%r4, %r3
	li	%r3, SIM_WRITE_CONSOLE_CODE
	.long	0x000eaeb0
	blr

/*
 * r3 - buffer
 * r4 - &len
 */
_GLOBAL(opal_write)
#define OPAL_CONSOLE_WRITE 1
	mr      %r5, %r3
	li      %r0, OPAL_CONSOLE_WRITE
	li      %r3, 0 /* terminal 0 */
	mflr    %r12
	std     %r12,STK_LR(%r1)
	ld      %r2, (kpcr_toc-kpcr)(%r13)
	LOAD_ADDR(%r12, opal_return)
	mtlr    %r12
	mfmsr   %r12
	li      %r11, MSR_LE
	andc    %r12, %r12, %r11
	mtspr   SPRN_HSRR1, %r12
	ld      %r2, (kpcr_opal_base-kpcr)(%r13)
	ld      %r12, (kpcr_opal_entry-kpcr)(%r13)
	mtspr   SPRN_HSRR0, %r12
	/*
	 * OPAL preserves r1, r13-r31. r3-r11 are
	 * arguments.
	 */
	hrfid
_GLOBAL(opal_return)
	FIXUP_ENDIAN
	ld      %r2, (kpcr_toc-kpcr)(%r13)
	ld      %r12,STK_LR(%r1)
	mtlr    %r12
	blr

/*
 * Toy per-cpu structure (for only 1 CPU).
 */
kpcr:
kpcr_toc:
	.llong 0
kpcr_opal_base:
	.llong 0
kpcr_opal_entry:
	.llong 0
