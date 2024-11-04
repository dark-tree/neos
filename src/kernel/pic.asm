
cpu 386
bits 32

PIC1_COMMAND equ 0x20
PIC1_DATA    equ 0x21
PIC2_COMMAND equ 0xA0
PIC2_DATA    equ 0xA1

section .text

extern con_write
extern gdtr_switch

global pic_disable

pic_disable:

	mov eax, 0xff
	out PIC1_DATA, al
	out PIC2_DATA, al

	ret
