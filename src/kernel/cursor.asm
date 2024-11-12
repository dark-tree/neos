
cpu 386
bits 32

section .text

global cur_enable
global cur_disable
global cur_goto

cur_enable:
	mov al, 0x0A
	mov dx, 0x3D4
	out dx, al

	mov dx, 0x3D5
	in al, dx
	and al, 0xC0
	or al, 1
	out dx, al

	mov al, 0x0B
	mov dx, 0x3D4
	out dx, al

	mov dx, 0x3D5
	in al, dx
	and al, 0xE0
	or al, 15
	out dx, al

	ret

cur_disable:
	mov al, 0x0A
	mov dx, 0x3D4
	out dx, al

	mov al, 0x20
	mov dx, 0x3D5
	out dx, al

	ret

cur_goto:
	mov ecx, [esp + 4]

	mov al, 0x0F
	mov dx, 0x3D4
	out dx, al

	mov al, cl
	mov dx, 0x3D5
	out dx, al

	mov al, 0x0E
	mov dx, 0x3D4
	out dx, al

	mov al, ch
	mov dx, 0x3D5
	out dx, al

	ret
