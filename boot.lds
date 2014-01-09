SECTIONS {
    . = 0x34000000;
    .text : { *(.text) }

	. = ALIGN(4);
    .rodata : { *(.rodata) } 

	. = ALIGN(4);
    .data : { *(.data) }

	. = ALIGN(4);
    __bss_start = .;
    .bss ALIGN(4)  : { *(.bss)  *(COMMON) }
    __bss_end = .;
}

