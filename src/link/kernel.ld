ENTRY(start)

/* Files that need to be linked together */
INPUT(build/kernel/entry.o)
INPUT(build/kernel/kmalloc.o)
INPUT(build/kernel/console.o)
INPUT(build/kernel/memory.o)

SECTIONS {
	. = 0x8000;

	/* Copy code and data for second stage */
	.text : SUBALIGN(0) {
		*(.text)
	}

	.data : SUBALIGN(4) {
		*(.rodata*)
		*(.data)
		*(.bss)
	}

	/* Remove unnecessary sections */
	/DISCARD/ : {
		*(.*);
	}
}
