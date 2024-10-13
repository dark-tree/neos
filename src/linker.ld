OUTPUT_FORMAT("binary")
ENTRY(start)

SECTIONS {
  . = 0x7e00;
  .text : { *(.text) }
  .data : { *(.data) }
}
