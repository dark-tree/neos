
cpu 386
bits 32

extern strlen

section .text

; See print.h
extern kprintf

; See pic.h
extern pic_disable

global halt
global dump
global panic

; For testing
global asm_test

halt:
	hlt
	jmp halt

panic:
	cli
	call pic_disable

	push dword [esp + 4] ; The panic message
	push panic_format
	call kprintf
	add esp, 8

	call dump

	panic_hcf:
		cli
		hlt
	jmp panic_hcf

dump:
	push ebp
	mov ebp, esp
	pusha

	push gs
	push fs
	push es
	push line_es_fs_gs

	push es
	push ds
	push cs
	push line_cs_ds_es

	push edi
	push esi
	push line_esi_edi

	push edx
	push ecx
	push line_ecx_edx

	push ebx
	push eax
	push line_eax_ebx

	call kprintf
	add esp, 12

	call kprintf
	add esp, 12

	call kprintf
	add esp, 12

	call kprintf
	add esp, 16

	call kprintf
	add esp, 16

	push line_begin
	call kprintf
	add esp, 4

	mov ebx, 0
	dump_show_line:

		mov eax, ebx
		shl eax, 4
		add eax, ebp

		push dword [eax + 3 * 4]
		push dword [eax + 2 * 4]
		push dword [eax + 1 * 4]
		push dword [eax + 0 * 4]
		push eax
		push line_stack
		call kprintf
		add esp, 6*4

		inc ebx

	cmp ebx, 8
	jnz dump_show_line

	popa
	pop ebp

	ret

asm_test:

	int 0x01
	call halt

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

	panic_format:        db `\e[1;31mKernel Panic had occured, restart the system to continue!\n%s\e[m\n\n`
	line_eax_ebx:  db `   \e[1;37mEAX\e[m: %#u0.8x, \e[1;37mEBX\e[m: %#u0.8x\n`, 0
	line_ecx_edx:  db `   \e[1;37mECX\e[m: %#u0.8x, \e[1;37mEDX\e[m: %#u0.8x\n`, 0
	line_esi_edi:  db `   \e[1;37mESI\e[m: %#u0.8x, \e[1;37mEDI\e[m: %#u0.8x\n\n`, 0
	line_cs_ds_es: db `   \e[1;37mCS\e[m: %#u0.4x, \e[1;37mDS\e[m: %#u0.4x, \e[1;37mSS\e[m: %#u0.4x\n`, 0
	line_es_fs_gs: db `   \e[1;37mES\e[m: %#u0.4x, \e[1;37mFS\e[m: %#u0.4x, \e[1;37mGS\e[m: %#u0.4x\n\n`, 0
	line_begin:    db "   ", 0xda, `\e[1;37mESP\e[m`, 0xc4, 0xc4, 0xc4, 0xbf, "  ", 0xda, `\e[1;37mEBP\e[m`, 0xc4, 0xc4, 0xc4, 0xbf, "  ", 0xda, `\e[1;37mRET\e[m`, 0xc4, 0xc4, 0xc4, 0xbf, `\n`, 0
	line_stack:    db `   \e[1;37m%u0.8x\e[m: %u0.8x  %u0.8x  %u0.8x  %u0.8x\n`, 0

	hello: db 0x9B, "1;35mHello Linux Compatible System!", 0x0A, 0x9B, "m", 0
