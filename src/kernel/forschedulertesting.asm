cpu 386
bits 32




section .text


extern kprintf

global getprocess1
global getprocess2







process1:
	mov EAX, [p1i]
	add EAX, 1
	mov [p1i], EAX
	push EAX
	mov EAX, process1text
	push EAX
	call kprintf
	add EAX, 4
	int 01h
jmp process1



times 2000 db 0




process2:
	mov EAX, [p2i]
	add EAX, 1
	mov [p2i], EAX
	push EAX
	mov EAX, process2text
	push EAX
	call kprintf
	add EAX, 8
	int 01h
jmp process2



times 2000 db 0

p1i: dd 0
p2i: dd 0

process1text: db `Im process 1! Hello! Iteration: %d! `, 0
process2text: db 'Im process 2! Hello! Iteration: %d!', 0




getprocess1:
	mov EAX, process1
ret


getprocess2:
	mov EAX, process2
ret

