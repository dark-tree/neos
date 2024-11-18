cpu 386
bits 32

section .text
extern kprintf


global process1
global process2



process1text: db 'P', 'r', 'o', 'c', 'e', 's', 's', ' ', '1', '\n', 0
process2text: db 'P', 'r', 'o', 'c', 'e', 's', 's', ' ', '2', '\n', 0




process1:
	mov EAX, process1text
	push EAX
	call kprintf
	add EAX, 4
jmp process1



times 2000 db 0




process2:
	mov EAX, process2text
	push EAX
	call kprintf
	add EAX, 4
jmp process2



times 2000 db 0





getprocess1:
	mov EAX, process1
ret


getprocess2:
	mov EAX, process2
ret

