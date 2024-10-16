OUTPUT_FORMAT("binary")
ENTRY(start)

/* Files that need to be linked together */
INPUT(build/kernel/entry.o)
INPUT(build/kernel/kmalloc.o)

SECTIONS {
    . = 0x8000;

    /* Copy code and data for second stage */
    .progtext : {
        *(.text)
    }

    .progdata : SUBALIGN(4) {
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
