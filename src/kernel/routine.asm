
cpu 386
bits 32

section .text

extern con_write
global isr_test

isr_test:
	pusha

	push dword 'X'
	call con_write
	add esp, 4

	popa
	iret
