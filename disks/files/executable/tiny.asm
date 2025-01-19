global _start

section .data:

	hello: db `Hello World!\n`
	path: db `./hello.txt`, 0

section .text:

_rip:
	pop eax
	mov ecx, eax
	sub eax, 5
	sub eax, _start
	jmp ecx

_start:

	call _rip
	push eax
	add eax, path
	mov ebx, eax

	; sys_open
	mov eax, 0x05
	mov ecx, 0x442
	mov edx, 0x180
	int 0x80

	; File Descriptor
	mov ebx, eax

	; Get pointer to "Hello World"
	pop ecx
	add ecx, hello

	; sys_write
	mov eax, 0x04
	mov edx, 13
	int 0x80

	; sys_exit
	mov eax, 0x01
	mov ebx, 0
	int 0x80
