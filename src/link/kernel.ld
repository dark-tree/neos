OUTPUT_FORMAT("binary")
ENTRY(start)

/* Files that need to be linked together */
INPUT(build/kernel/entry.o)

SECTIONS {
	. = 0x8000;


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
