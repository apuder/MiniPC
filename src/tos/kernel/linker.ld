ENTRY(_start)

__kernel_load_addr = 0x40000000;
__stack_top = 0x40800000;

SECTIONS
{
	. = __kernel_load_addr;
	__image_start = .;

	.text : ALIGN(4)
	{
		*(.text .text.*)
	}

	.rodata : ALIGN(4)
	{
		*(.rodata .rodata.*)
		*(.srodata .srodata.*)
	}

	.data : ALIGN(4)
	{
		__data_start = .;
		*(.data .data.*)
		__sdata_start = .;
		*(.sdata .sdata.*)
		__data_end = .;
	}
	__data_load_start = LOADADDR(.data);
	PROVIDE(__global_pointer$ = __sdata_start + 0x800);

	.bss (NOLOAD) : ALIGN(4)
	{
		__bss_start = .;
		*(.sbss .sbss.*)
		*(.bss .bss.*)
		*(COMMON)
		__bss_end = .;
	}

	. = ALIGN(4);
	__image_end = .;
	__image_size = __image_end - __image_start;
	_end = .;
}
