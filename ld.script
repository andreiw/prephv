ENTRY(_start)
SECTIONS
{
	.text :
	{
		_start = .;
		*(.text)
		. = ALIGN(4096);
		*(.rodata*)
		*(.data*)
		*(.sdata*)
		*(.got2)
		*(.toc1)
		*(.branch_lt)
		. = ALIGN(4096);
		*(.opd)
		__toc_start = .;
		*(.got)
		*(.toc)
		. = ALIGN(4096);
		_edata  =  .;
		__bss_start = .;
		*(.sbss)
		*(.bss)
		*(COMMON)
		_end = . ;
	}
}
