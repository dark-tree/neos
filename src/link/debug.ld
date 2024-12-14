ENTRY(start)

SECTIONS {
	. = 0x8000;

	/* First output all the functions */
	.text : SUBALIGN(0) {
		*(.text.start)
		*(.text)
		*(.text.unlikely)
	}

	/* And then all the data */
	.data : SUBALIGN(4) {
		*(.rodata*)
		*(.data)
		*(.bss)
	}

}
