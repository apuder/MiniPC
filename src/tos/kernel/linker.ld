/* Minimal linker script to create an ELF file with two program headers */

OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

PHDRS
{
  text PT_LOAD FLAGS(5);   /* Read + Execute */
  rodata PT_LOAD FLAGS(4); /* Read-only */
}

SECTIONS
{
  /* Read + Execute segment */
  .text :
  {
    *(.text)       /* Include all .text sections from input files */
    *(.text.*)     /* Include all other sections starting with .text */
    _etext = .;    /* Mark end of text segment */
  } :text

  /* Read-only segment */
  .rodata :
  {
    *(.rodata)     /* Include all .rodata sections from input files */
    *(.rodata.*)   /* Include all other sections starting with .rodata */
    _erodata = .;  /* Mark end of rodata segment */
  } :rodata

  /* Start of data segment */
  .data :
  {
    *(.data)       /* Include all .data sections */
    *(.data.*)     /* Include all other sections starting with .data */
    _edata = .;    /* Mark end of data segment */
  }

  /* BSS segment */
  .bss :
  {
    *(.bss)        /* Include all .bss sections */
    *(.bss.*)      /* Include all other sections starting with .bss */
    _end = .;      /* Mark end of BSS */
  }
}

