
cpu 386
bits 32

; Some usused port that we write to make sure we don't overwhelm our poor PIC, linux uses 0x80
UNUSED    equ 0x80

; Master PIC
PIC1_COM  equ 0x20
PIC1_DAT  equ 0x21

; Slave
PIC2_COM  equ 0xA0
PIC2_DAT  equ 0xA1

PIC_EOI   equ 0x20 ; End Of Interrupt Command
PIC_INI   equ 0x11 ; PIC will then expect 3 control words on data port
PIC_ISR   equ 0x0b ; In-service registers of the PIC (bit mask of active IRQs)

section .text

global pic_disable
global pic_enable
global pic_remap
global pic_isr
global pic_irq
global pic_accept

pic_accept:

	; Load IRQ bitmask
	mov edx, [esp+4]

	; End Of Interrupt Command
	mov al, PIC_EOI

	; Handle slave
	test dh, dh
	jz pic_accept_master

		out PIC2_COM, al

	; Handle master
	pic_accept_master:
	test dl, dl
	jz pic_accept_exit

		out PIC1_COM, al

	pic_accept_exit:
	ret

; Check if the given interrupt was caused by IRQ
; Returns IRQ mask (0x01 - 0x80) if yes, 0 otherwise
pic_irq:

	; Load interrupt number and our PIC offset
	mov ecx, [esp + 4]
	mov edx, [pic_offset]

	; Calculate which IRQ it was by substracting the base offset
	sub ecx, edx

	; If the interrupt < pic_offset (negative result) it was NOT an IRQ
	jb pic_irq_invalid

	; There are only 16 IRQs so if we got IRQ >= 16 it was not an IRQ
	cmp ecx, 16
	jae pic_irq_invalid

	; Return 1 based IRQ number
	mov eax, 1
	shl eax, cl
	ret

	; This was not an IRQ, return 0
	pic_irq_invalid:
	xor eax, eax
	ret

; Returns a combined, 16-bit, in-service register
pic_isr:

	xor eax, eax

	; Load and shift mask from slave
	mov al, PIC_ISR
	out PIC2_COM, al
	in al, PIC2_COM
	mov ah, al

	; Load mask from master
	mov al, PIC_ISR
	out PIC1_COM, al
	in al, PIC1_COM

	ret

pic_remap:

	; Load (and save) base interrupt offset number
	mov ecx, [esp + 4]
	mov [pic_offset], ecx

	; Calculate slave offset
	lea edx, [ecx + 8]

	; Command PIC_INI makes PIC wait for
	; 3 control words on their data input ports
	mov al, PIC_INI
	out PIC1_COM, al
	out UNUSED, al
	out PIC2_COM, al
	out UNUSED, al

	; First value is the IVT offset
	mov al, cl
	out PIC1_DAT, al
	out UNUSED, al

	; Place slave just after master
	mov al, dl
	out PIC2_DAT, al
	out UNUSED, al

	; Second value tells the PIC how they are connected
	mov al, 0x04
	out PIC1_DAT, al
	out UNUSED, al
	mov al, 0x02
	out PIC2_DAT, al
	out UNUSED, al

	; and third sets aditional flags, we will use 8086 mode
	mov al, 0x01
	out PIC1_DAT, al
	out UNUSED, al
	out PIC2_DAT, al
	out UNUSED, al

	ret

; Enable all interrupt lines
pic_enable:
	mov al, 0x00
	out PIC1_DAT, al
	out PIC2_DAT, al

	ret

; Disable all interrupt lines
pic_disable:
	mov al, 0xff
	out PIC1_DAT, al
	out PIC2_DAT, al

	ret

section .data

	; PIC interrupt number offset
	pic_offset: dd 0
