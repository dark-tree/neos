
cpu 386
bits 32

section .text

global halt

halt:
	hlt
	jmp halt
