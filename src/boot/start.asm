
cpu 386
bits 16

extern prints
extern printd
extern printn
extern printl
extern halt
extern fault

section .text

start:

	cli

	; Load temporary global descriptor table
	lgdt ds:[gdt_desc]

	; Enter protected mode
	mov eax, cr0
	or eax, 1
	mov cr0, eax

	; Point all the segments at the data GDT entry
	mov ax, 0x10
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	; Set the code segment using a far-jump
	jmp 0x08:0x8000

section .data

	; + ---------- + -- + -- + -- + -- + ----------- + -- + ----- + -- + ---- + ---------- +
	; | 31      24 | 23 | 22 | 21 | 20 | 19       16 | 15 | 14 13 | 12 | 11 8 | 7        0 |
	; + ---------- + -- + -- + -- + -- + ----------- + -- + ----- + -- + ---- + ---------- + (+4)
	; | Base 31:24 | Gr | Db | Lo | Av | Limit 19:16 | Pr | Ring  | Dt | Type | Base 23:16 |
	; + -------------------------------------------- + ----------------------------------- +
	; | 31                                        16 | 15                                0 |
	; + -------------------------------------------- + ----------------------------------- + (+0)
	; | Base 15:00                                   | Limit 15:00                         |
	; + -------------------------------------------- + ----------------------------------- +
	;
	; Gr - Granularity, implicitly multiply limit by 1024
	; Db - Default operation size, 0=16bit/1=32bit
	; Lo - Long, marks 64bit code segments
	; Av - Available, Can be used by the OS
	; Pr - Present, marks the existance of this segment in memory
	; Dt - Descriptor type, 0=System, 1=Code/Data
	;
	; Type  -  4 bit
	; Ring  -  2 bit, 00=Most Privileged, 11=Least Privileged
	; Limit - 20 bit
	; Base  - 32 bit

	gdt_null:
		dd 0x00000000
		dd 0x00000000

	gdt_code:
		dd 0x0000FFFF
		dd 0x00CF9B00

	gdt_data:
		dd 0x0000FFFF
		dd 0x00CF9300

	gdt_desc:
		dw gdt_desc - gdt_null
		dd gdt_null
