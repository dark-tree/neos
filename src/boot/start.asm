
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

	; Download the memory map from the BIOS
	call memmap

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

memmap:

	; Initial continuation value, BIOS defined query entry identifier
	; All we know is that each time we run int 0x15, AX=0xE820 we should pass
	; the continuation returned by the previous call unmodified, or pass 0 if this is the first call
	xor ebx, ebx

	; Reset segment to 0, as BIOS will write to ES:DI
	mov es, bx

	; The starting address of the memory map
	mov di, 0x500

	; first dword will be used to store length of the memory map
	add di, 4

	; Start length at 1, we incremnt after learning that there are more entries not before
	mov dword es:[0x500], 0

	memmap_load:

		; Fill basic static function information
		mov eax, 0xE820     ; BIOS "Query System Address Map" function identifier
		mov ecx, 20         ; Maximum number of bytes to fill, 20 is the minimum
		mov edx, 0x534D4150 ; Magic Value

		; Make the call
		int 0x15

		; Bios signifies end of memory map by either setting carry
		; or retruning 0 as the continuation value (EBX)
		jc memmap_done

		test ebx, ebx
		jz memmap_done

		; Move to the next entry
		add di, 20

		inc dword es:[0x500]

	jmp memmap_load

	memmap_done:
	ret

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
