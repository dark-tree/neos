global _start

section .data:

	hello: db `Hello World!\n`
	path: db `./hello.txt`, 0

section .text:

_start:

	; sys_open
	mov eax, 0x05
	mov ebx, path
	mov ecx, 0x442
	mov edx, 0x180
	int 0x80

	; File Descriptor
	mov ebx, eax

	; sys_close
	mov eax, 0x06
	int 0x80

	jmp _start
