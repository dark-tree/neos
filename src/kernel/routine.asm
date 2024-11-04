
cpu 386
bits 32

section .text

extern con_write
extern gdtr_switch

global isr_register
global isr_init
global isr_wrap

%macro define_isr 2
	push dword 1
	push isr_head%1
	push %1
	call isr_wrap
	add esp, 12
	jmp %%define_isr%1_skip

	isr_head%1:
		cli
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
	mov eax, [esp+44]

	; Interrupt number
	mov ebx, [esp+40]

	; Load handler pointer
	mov ecx, [service_table + ebx * 4]

	; Call kernel handler procedure
	test ecx, ecx
	jz isr_tail_no_handle
		push eax
		push ebx
		call ecx
		add esp, 8

	isr_tail_no_handle:

	; This alignes with the saved segments from before
	call gdtr_switch
	add esp, 8

	; Pop everything back
	popa

	; Pop ISR number and error code
	add esp, 8

	; We need to renable interupts
	sti
	iret

isr_register:
	mov edx, [esp+8] ; Function pointer
	mov eax, [esp+4] ; Interrupt number

	; Write function to handler array
	mov [service_table + eax * 4], edx

	ret

; idt_write(int index, void* offset, int gdt_index)
isr_wrap:

	mov edx, [esp+8] ; Interupt Service Routine pointer
	mov ecx, [esp+4] ; Interupt number

	; Calculate entry index
	lea eax, [ecx * 8]

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

	;            int, hec
	define_isr  0x00,   0 ; Divide by 0
	define_isr  0x01,   0 ; Reserved
	define_isr  0x02,   0 ; NMI Interrupt
	define_isr  0x03,   0 ; Breakpoint (INT3)
	define_isr  0x04,   0 ; Overflow (INTO)
	define_isr  0x05,   0 ; Bounds range exceeded (BOUND)
	define_isr  0x06,   0 ; Invalid opcode (UD2)
	define_isr  0x07,   0 ; Device not available (WAIT/FWAIT)
	define_isr  0x08,   1 ; Double fault
	define_isr  0x09,   0 ; Coprocessor segment overrun
	define_isr  0x0A,   1 ; Invalid TSS
	define_isr  0x0B,   1 ; Segment Not Present
	define_isr  0x0C,   1 ; Stack-Segment Fault
	define_isr  0x0D,   1 ; General Protection Fault
	define_isr  0x0E,   1 ; Page Fault
	define_isr  0x0F,   0 ; Reserved
	define_isr  0x10,   0 ; x87 Floating-Point Exception
	define_isr  0x11,   1 ; Alignment Check
	define_isr  0x12,   0 ; Machine Check
	define_isr  0x13,   0 ; SIMD Floating-Point Exception
	define_isr  0x14,   0 ; Virtualization Exception
	define_isr  0x15,   1 ; Control Protection Exception
	define_isr  0x16,   0 ; Reserved
	define_isr  0x17,   0 ; Reserved
	define_isr  0x18,   0 ; Reserved
	define_isr  0x19,   0 ; Reserved
	define_isr  0x1A,   0 ; Reserved
	define_isr  0x1B,   0 ; Reserved
	define_isr  0x1C,   0 ; Hypervisor Injection Exception
	define_isr  0x1D,   0 ; VMM Communication Exception
	define_isr  0x1E,   0 ; Security Exception
	define_isr  0x1F,   0 ; Reserved

	define_isr  0x70,   0 ; IRQ 8:  RTC
	define_isr  0x71,   0 ; IRQ 9:  Unassigned
	define_isr  0x72,   0 ; IRQ 10: Unassigned
	define_isr  0x73,   0 ; IRQ 11: Unassigned
	define_isr  0x74,   0 ; IRQ 12: Mouse controller
	define_isr  0x75,   0 ; IRQ 13: Math coprocessor
	define_isr  0x76,   0 ; IRQ 14: Hard disk controller 1
	define_isr  0x77,   0 ; IRQ 15: Hard disk controller 2
	define_isr  0x80,   0 ; Syscall

	mov esp, ebp
	pop ebp
	ret


section .data

service_table:
	times 256 dd 0
