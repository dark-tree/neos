
cpu 386
bits 32

extern strlen

section .text

global halt
global asm_test

halt:
	hlt
	jmp halt

asm_test:
	push hello
	call strlen
	add esp, 4

	; Invoke Linux syscall 'sys_write'
	mov edx, eax   ; String length
	mov eax, 0x04  ; Write
	mov ebx, 0     ; File Descriptor (ignored)
	mov ecx, hello ; Buffer pointer
	int 0x80
	ret

section .data

hello: db 0x9B, "1;35mHello Linux Compatible System!", 0x0A, 0x9B, "m", 0
