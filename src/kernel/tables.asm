
cpu 386
bits 32

section .text

global idtr_store
global gdtr_store
global gdtr_switch

; Modifies the IDTR register to point to the given interupt table,
; this function will also reenable interrupts if the limit is grater than 0
; Signature: void abi_cdecl idtr_store(int offset, int limit);
idtr_store:

	cli

	; Those two registers need not be preserved as per CDECL
	mov edx, [esp+8] ; Limit
	mov eax, [esp+4] ; Offset

	; But we will use the stack so save EBP
	push ebp
	mov ebp, esp

	; Allocate space for the 6-byte-long IDTR on stack
	sub esp, 6

	; Construct the IDTR in memory
	mov word  [esp + 0], dx
	mov dword [esp + 2], eax

	; Switch to the new interupt table
	lidt [esp]

	; If the limit was not 0, enable interrupts
	test dx, dx
	jz idtr_store_exit
		sti
	idtr_store_exit:

	mov esp, ebp
	pop ebp
	ret

; Modifies the GDTR register to point to the given descriptor table,
; Use gdtr_switch(int data, int code) to select a specific GDT entry
; Signature: void abi_cdecl gdtr_store(int offset, int limit);
gdtr_store:

	; Those two registers need not be preserved as per CDECL
	mov edx, [esp+8] ; Limit
	mov eax, [esp+4] ; Offset

	; But we will use the stack so save EBP
	push ebp
	mov ebp, esp

	; Allocate space for the 6-byte-long GDTR on stack
	sub esp, 6

	; Construct the GDTR in memory
	mov word  [esp + 0], dx
	mov dword [esp + 2], eax

	; Switch to the new interupt table
	lidt [esp]

	mov esp, ebp
	pop ebp
	ret

; Switches all segments to the given GDT index pair,
; The GDT must have been stored into GDTR first using gdtr_store(int offset, int limit).
; Signature: void abi_cdecl gdtr_switch(int data, int code);
gdtr_switch:

	; Those two registers need not be preserved as per CDECL
	mov edx, [esp+8] ; Code segment index
	mov eax, [esp+4] ; Data segment index

	; Shift indices left by 3 bits so that RPL and TI are both 0
	shl edx, 3
	shl eax, 3

	; Point all the 'data' segments at the data GDT entry
	mov ds, ax
	mov ss, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	; Commence SpoOoOoky Witchcraft :D
	push dword edx
	push dword [esp + 4]
	retf 4
