OUTPUT_FORMAT("elf32-i386")
INPUT(build/boot/load.o)

SECTIONS {
	. = 0x7c00;

	/* Reference the boot section in memory */
	.bootlib (NOLOAD) : {
		build/boot/load.o(.text)
		build/boot/load.o(.data)
		build/boot/load.o(.bss)
	}

	. = 0x7e00;

	/* Copy code and data for second stage */
	.prog : SUBALIGN(4) {
		build/boot/start.o(.text)
		build/boot/start.o(.rodata*)
		build/boot/start.o(.data)
		build/boot/start.o(.bss)
	}

	/* Remove unnecessary sections */
	/DISCARD/ : {
		*(.eh_frame);
		*(.rel.eh_frame);
		*(.rela.eh_frame);
		*(.comment);
	}
}
