OUTPUT_FORMAT("elf32-i386")

SECTIONS {
	. = 0x7c00;

	/* Copy code and data for second stage */
	.prog : SUBALIGN(4) {
		*(.text)
		*(.rodata*)
		*(.data)
		*(.bss)
	}

	/* Remove unnecessary sections */
	/DISCARD/ : {
		*(.eh_frame);
		*(.rel.eh_frame);
		*(.rela.eh_frame);
		*(.comment);
	}
}
