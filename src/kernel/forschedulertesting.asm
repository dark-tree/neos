cpu 386
bits 32




section .text


extern kprintf

global getprocess1
global getprocess2







process1:
	mov EAX, 0
proc1ptl:
	add EAX, 1
	push EAX
	mov ECX, process1text
	push ECX
	call kprintf
	add ESP, 4
	pop EAX

jmp proc1ptl



times 2000 db 0




process2:
	mov EAX, 0
proc2ptl:
	add EAX, 1
	push EAX
	mov ECX, process2text
	push ECX
	call kprintf
	add ESP, 4
	pop EAX

jmp proc2ptl



times 2000 db 0

process1text: db `Im process 1! Hello! Iteration: %d! `, 0
process2text: db 'Im process 2! Hello! Iteration: %d!', 0




getprocess1:
	mov EAX, process1
ret


getprocess2:
	mov EAX, process2
ret

