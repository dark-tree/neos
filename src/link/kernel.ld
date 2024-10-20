ENTRY(start)

/* Files that need to be linked together */
INPUT(build/kernel/entry.o)
INPUT(build/kernel/kmalloc.o)
INPUT(build/kernel/console.o)
INPUT(build/kernel/memory.o)
INPUT(build/kernel/math.o)

SECTIONS {
	. = 0x8000;

	/* First output all the functions */
	.text : SUBALIGN(0) {
		*(.text)
	}

	/* And then all the data */
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
