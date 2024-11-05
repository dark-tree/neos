
cpu 386
bits 32

section .text

; See tables.h
extern gdtr_switch

; See pic.h
extern pic_remap
extern pic_isr
extern pic_irq
extern pic_accept

; See interrupt.c
extern int_common_handle

global isr_name
global isr_register
global isr_init

%macro define_isr 3
	push dword 1
	push isr_head%1
	push %1
	call isr_wrap
	add esp, 12
	mov dword [name_table + %1 * 4], isr_name%1
	jmp %%define_isr%1_skip

	isr_name%1: db %3, 0

	isr_head%1:
		%if %2 = 0
			push dword 0
		%endif

		push dword %1
		jmp isr_tail

	%%define_isr%1_skip:
%endmacro

isr_tail:
	; Pushes edi, esi, ebp, esp, ebx, edx, ecx, eax
	pusha

	; Pointer to the extended parameter block
	mov ebp, esp

	; Clear high 16 bits so nothing happens to our segments
	xor eax, eax

	; Save code segment index
	mov ax, cs
	shr ax, 3
	push eax

	; Save data segment index
	mov ax, ds
	shr ax, 3
	push eax

	; Switch to kernel mode
	push dword 1
	push dword 2
	call gdtr_switch
	add esp, 8

	; Interrupt error code
	mov edx, [esp + 44]

	; Interrupt number
	mov eax, [esp + 40]

	; Load CDECL handler pointer, we use a preserved register here
	mov esi, [service_table + eax * 4]

	; Used by int_common_handler
	push eax

	; Those will become the handler call arguments
	push dword [ebp + 4 * 0] ; EDI
	push dword [ebp + 4 * 1] ; ESI
	push dword [ebp + 4 * 4] ; EBX
	push dword [ebp + 4 * 5] ; EDX
	push dword [ebp + 4 * 6] ; ECX
	push dword [ebp + 4 * 7] ; EAX

	push edx
	push eax

	; Convert interrupt number to IRQ bitmask (result in EAX)
	; Uses the same stack element (last push) that will be later used during CDECL call so we don't pop
	call pic_irq

	; EAX now contains the IRQ bitmask
	test eax, eax
	jz isr_tail_not_irq

		; Save eax, as we will need it later
		; We will restore it to a diffrent register
		push eax

		; This takes no args
		; EAX will contain the PIC's ISR
		call pic_isr

		; We now have the ISR mask in EAX
		; and the IRQ mask in EBX (we use this register as it will be preserved after the C call)
		pop ebx

		; Check if IRQ mask and ISR match
		; there should be a single shared bit for all valid IRQs
		and ebx, eax
		pushf

		; If some bit was set in high 8 bits, set any bit (we set the correct IRQ2 bit) in the low 8 bits
		; This will be used to determine to which PIC should the EOI command be send after the handler
		test bh, bh
		jz isr_tail_no_low_bit

			or bx, 0x02

		isr_tail_no_low_bit:

		; Return to the result of the AND, if there were no bits in common this was
		; a spurious interrupt so we will skip the handler
		popf
		jz isr_tail_no_handle

	isr_tail_not_irq:

	; Call kernel handler procedure
	test esi, esi
	jz isr_tail_no_handle

		; This alignes with EDX/EAX pair pushed earlier
		call esi

	isr_tail_no_handle:

	; Arguments were pushed before so we always pop them here
	add esp, 4*8

	; Call this always, used by the "wait" subsystem
	call int_common_handle
	add esp, 4

	; This alignes with the saved segments from before
	call gdtr_switch
	add esp, 8

	push ebx
	call pic_accept
	add esp, 4

	; Pop everything back
	popa

	; Pop ISR number and error code (pushed in head)
	add esp, 8

	iret

isr_name:
	mov eax, [esp+4] ; Interrupt number
	mov eax, [name_table + eax * 4]
	ret

isr_register:
	mov edx, [esp+8] ; Function pointer
	mov eax, [esp+4] ; Interrupt number

	; Write function to handler array
	mov [service_table + eax * 4], edx

	ret

; This function doesn't clobber the arguments
isr_wrap:

	mov eax, [esp+16] ; Address of the Interrupt Descriptor Table
	mov edx, [esp+8]  ; Interupt Service Routine pointer
	mov ecx, [esp+4]  ; Interupt number

	; Calculate entry index
	lea eax, [eax + ecx * 8]

	; Copy GDT index into IDT entry
	movzx ecx, word [esp+12]
	shl ecx, 3
	mov word [eax+2], cx

	; Attributes and low-bytes of the offset
	mov byte [eax+5], 0xEE
	mov byte [eax+4], 0
	mov word [eax], dx

	; Copy high bytes
	shr edx, 16
	mov word [eax+6], dx
	ret

isr_init:

	push ebp
	mov ebp, esp

	; Load IDT offset onto the stack, this will be used during isr_wrap calls in define_isr
	; This is technically not allowed by CDECL but we will do it here anyway
	push dword [esp+8]

	;            int, e  name
	define_isr  0x00, 0, "Division Error"
	define_isr  0x01, 0, "Debug"
	define_isr  0x02, 0, "Non-Maskable Interrupt"
	define_isr  0x03, 0, "Breakpoint"
	define_isr  0x04, 0, "Overflow"
	define_isr  0x05, 0, "Bound Exceeded"
	define_isr  0x06, 0, "Invalid Opcode"
	define_isr  0x07, 0, "Device Not Available"
	define_isr  0x08, 1, "Double Fault"
	define_isr  0x09, 0, "Coprocessor Segment Overrun"
	define_isr  0x0A, 1, "Invalid TSS"
	define_isr  0x0B, 1, "Segment Not Present"
	define_isr  0x0C, 1, "Stack-Segment Fault"
	define_isr  0x0D, 1, "General Protection Fault"
	define_isr  0x0E, 1, "Page Fault"
	define_isr  0x0F, 0, "Reserved"
	define_isr  0x10, 0, "x87 Floating-Point Exception"
	define_isr  0x11, 1, "Alignment Check"
	define_isr  0x12, 0, "Machine Check"
	define_isr  0x13, 0, "SIMD Floating-Point Exception"
	define_isr  0x14, 0, "Virtualization Exception"
	define_isr  0x15, 1, "Control Protection Exception"
	define_isr  0x16, 0, "Reserved"
	define_isr  0x17, 0, "Reserved"
	define_isr  0x18, 0, "Reserved"
	define_isr  0x19, 0, "Reserved"
	define_isr  0x1A, 0, "Reserved"
	define_isr  0x1B, 0, "Reserved"
	define_isr  0x1C, 0, "Hypervisor Injection Exception"
	define_isr  0x1D, 0, "VMM Communication Exception"
	define_isr  0x1E, 0, "Security Exception"
	define_isr  0x1F, 0, "Reserved"

	push 0x20
	call pic_remap
	add esp, 4

	;            int, e, name
	define_isr  0x20, 0, "IRQ 0: Timer"
	define_isr  0x21, 0, "IRQ 1: Keyboard"
	define_isr  0x22, 0, "IRQ 2: Slave"
	define_isr  0x23, 0, "IRQ 3: COM2, COM4"
	define_isr  0x24, 0, "IRQ 4: COM1, COM3"
	define_isr  0x25, 0, "IRQ 5: LPT2"
	define_isr  0x26, 0, "IRQ 6: Floppy controller"
	define_isr  0x27, 0, "IRQ 7: LPT1"
	define_isr  0x28, 0, "IRQ 8: Clock"
	define_isr  0x29, 0, "IRQ 9: ACPI"
	define_isr  0x2A, 0, "IRQ 10: Unassigned"
	define_isr  0x2B, 0, "IRQ 11: Unassigned"
	define_isr  0x2C, 0, "IRQ 12: Mouse"
	define_isr  0x2D, 0, "IRQ 13: Coprocessor"
	define_isr  0x2E, 0, "IRQ 14: Primary ATA"
	define_isr  0x2F, 0, "IRQ 15: Secondary ATA"

	;            int, e, name
	define_isr  0x80, 0, "Linux Syscall"

	mov esp, ebp
	pop ebp
	ret


section .data

service_table:
	times 256 dd 0

name_table:
	times 256 dd 0
