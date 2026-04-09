ENTRY(_start)

__kernel_load_addr = 0x40000000;
__small_data_start = 0x00000000;
__small_data_size = 0x00001000;
__stack_top = 0x40800000;

SECTIONS
{
	. = __kernel_load_addr;
	__image_start = .;


	.startup : ALIGN(4)
	{
		KEEP(*(.startup))
	}

	. = __kernel_load_addr + 0x80;
	.irq_vector : ALIGN(4)
	{
		KEEP(*(.irq_vector))
	}

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
		. = ALIGN(4);
		__data_end = .;
	}
	__data_load_start = LOADADDR(.data);

	.sdata __small_data_start : AT(LOADADDR(.data) + SIZEOF(.data))
	{
		__sdata_start = .;
		*(.sdata .sdata.*)
		. = ALIGN(4);
		__sdata_end = .;
	}
	__sdata_load_start = LOADADDR(.sdata);
	__sdata_load_end = LOADADDR(.sdata) + SIZEOF(.sdata);

	.sbss (NOLOAD) : ALIGN(4)
	{
		__sbss_start = .;
		*(.sbss .sbss.*)
		*(.scommon)
		. = ALIGN(4);
		__sbss_end = .;
	}
	__small_data_end = __sbss_end;
	__global_pointer$ = __small_data_start + (__small_data_size / 2);

	. = __data_end;
	. = ALIGN(4);

	.bss (NOLOAD) : ALIGN(4)
	{
		__bss_start = .;
		*(.bss .bss.*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end = .;
	}

	__image_end = __sdata_load_end;
	__image_size = __image_end - __image_start;
	_end = __bss_end;
}
