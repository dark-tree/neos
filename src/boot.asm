
cpu 386
[bits 16]

%define err_guard 1 ; Bootloader integrity self-check failed
%define err_scan  2 ; "Get Current Drive Parameters" BIOS call failed
%define err_load  3 ; "Read Sectors into Memory" BIOS call failed
%define err_a20   4 ; Failed to anable A20 gate

section .text
[org 0x7c00]

main:

	cld

	; BIOS writes the boot drive number into DL before
	; jumping to 0x7c00, we need to preserve that so that we
	; can later read the kernel
	mov [drive_handle], dl

	; 80x25 Color Text Mode
	mov ah, 0
	mov al, 0x3
	int 0x10

	; Zero-out DS and ES (why zeroing CS breaks?)
	xor ax, ax
	mov ds, ax
	mov es, ax
	mov ss, ax

	; Put stack before main
	mov ax, main
	mov sp, ax

	; Make sure the bootloadred wasn't truncated
	mov al, [guard]
	mov ah, 42

	cmp al, ah
	je main_ok

		; Bootloader self-check failed, print error and halt
		mov ax, err_guard
		call fault

	main_ok:

	; Print boot drive
	mov si, str_boot_drive
	call prints

	; Print drive handle
	xor ah, ah
	mov al, [drive_handle]
	call printd
	call printn

	; Get drive geometry
	call scan

	; Copy drive into RAM
	call load

	; Enable A20 gate
	call a20



	; Wait forever
	main_stop:
		cli
		hlt
	jmp main_stop

a20:

	; Access to 0xEE enables A20 on some systems
	; As we need just one instruction to do this we might as well
	in al, 0xEE

	; Try enabling A20 using the keyboard controller
	; Disable keyboard
	call kwait
	mov al, 0xAD
	out 0x64, al

	; Set the keyboard controller output port
	call kwait
	mov al, 0xD1
	out 0x64, al

	; Enable all bits in Controller Output Port, including A20
	call kwait
	mov al, 0xFF
	out 0x60, al

	; Enable keyboard
	call kwait
	mov al, 0xAE
	out 0x64, al

; Wait for keyboard to report ready for a write to 0x64/0x60
kwait:
	in al, 0x64 ; Read keyboard status register
	test al, 2  ; Check input buffer status
	jnz kwait   ; Retry until full
	ret

; Reads current drive parameters into global bootloader variables
scan:
	call printn

	mov ah, 0x08
	mov dl, [drive_handle]

	int 0x13
	jnc scan_ok

		; Print error message and halt
		mov ax, err_scan
		call fault

	scan_ok:

	; Two high cylinders bits are stored in DH, get rid of them
	and dh, 0x3F

	; BIOS returns the MAXIMUM value not the COUNT so we need to add one
	; As Sectors are 1-based we don't need to remap them
	inc dh
	inc ch

	mov [drive_sectors], cl
	mov [drive_heads], dh
	mov [drive_cylinders], ch

	; decode bytes per sector from enum to value
	mov bx, 128
	mov cl, es:[di + 0x03]
	shl bx, cl
	mov [drive_stride], bx

	; Clear AX and the returned pointer
	xor ax, ax

	mov si, str_sectors
	mov al, [drive_sectors]
	call printl

	mov si, str_heads
	mov al, dh
	call printl

	mov si, str_cylinders
	mov al, ch
	call printl

	mov si, str_stride
	mov ax, bx
	call printl

	ret

; Loads and copies the data from the drive, call after 'scan'
load:
	call printn

	mov di, 0x7c0 ; Destination Segment
	mov si, 0     ; Source Sector Index

	load_next:

		; Convert stride to segment value offset
		mov ax, [drive_stride]
		shr ax, 4
		add di, ax

		; move to the next sector
		inc si
		mov ax, si

		; Divide AX by sector count, result in AL, reminder in AH
		; so we get head count in AL and sector in AH
		div byte [drive_sectors]

		; Put Sector into the target register
		mov cl, ah
		xor ah, ah

		; Divide AX by head count, result in AL, reminder in AH
		; so we get cylinder count in AL and head in AH
		div byte [drive_heads]

		; Copy values to the expected registers
		mov bx, 0  ; Offset
		mov dh, ah ; Head
		mov ch, al ; Cylinder

		; Sectors are 1-indexd
		inc cl

		pusha
			mov ax, si
			mov si, str_copying

			; AX will be preserved so we can print it like this
			call prints
			call printd
		popa

		mov es, di  ; Copy segment
		mov ah, 0x2 ; Read Sectors into Memory
		mov al, 1   ; Read one sector (512 bytes, 0x200)
		mov dl, [drive_handle]

		int 0x13
		jnc load_ok

			; Print error message and halt
			mov ax, err_load
			call fault

		load_ok:

	cmp si, 70
	jnz load_next

	call printn
	ret

; Prints boot error message with code AX and halts
fault:
	call printn
	mov si, str_fault
	call printl

	; Wait forever
	fault_halt:
		cli
		hlt
	jmp fault_halt

; Prints DS:SI null-terminated string folowed by AX decimal number and new line
printl:
	call prints
	call printd
	call printn
	ret

; Prints "\n\r" to move to the begining of the next line
printn:
	push ax
	mov ah, 0x0e

	; new line
	mov al, 10
	int 0x10

	; carrige return
	mov al, 13
	int 0x10

	pop ax
	ret

; Prints DS:SI null-terminated string to screen
prints:
	pusha
	mov ah, 0x0e

	prints_next:
		lodsb
		test al, al
		jz prints_exit

		int 0x10
		jmp prints_next

	prints_exit:
	popa
	ret

; Prints AX to screen as a decimal number
printd:
	pusha
	sub sp, 16
	mov bp, sp
	mov cx, 10

	mov [bp], byte 0
	inc bp

	printd_next:
		xor dx, dx
		div cx

		; Convert to ASCII digit and store in buffer
		add dl, 0x30
		mov [bp], dl
		inc bp

		test ax, ax
	jnz printd_next

	; Print resulting digit string
	mov ah, 0x0e

	; Write
	printd_load:
		dec bp
		mov al, BYTE ss:[bp]

		; Exit if NULL byte is loaded
		test al, al
		jz printd_exit

		; Print and goto next character
		int 0x10
	jmp printd_load

	printd_exit:
	add sp, 16
	popa
	ret

section .data

	; Global variables loaded by the bootloader
	drive_handle:    db 0
	drive_sectors:   db 0
	drive_heads:     db 0
	drive_cylinders: db 0
	drive_stride:    dw 0

	; Strings used by the bootloader
	str_fault:       db "Boot Fault: ", 0
	str_boot_drive:  db "Boot Drive: ", 0
	str_copying:     db 13, "Copying ", 0
	str_of:          db " of ", 0
	str_sectors:     db " * Sectors   : ", 0
	str_heads:       db " * Heads     : ", 0
	str_cylinders:   db " * Cylinders : ", 0
	str_stride:      db " * Stride    : ", 0

	; Checked on start to make sure the bootlaoder wasn't truncated
	guard:           db 42
