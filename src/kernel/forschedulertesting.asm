cpu 386
bits 32

extern dump
extern panic


section .text


extern kprintf

global getprocess1
global getprocess2







process1:
	mov EAX, 0

proc1ptl:

	; open("abc/foo/test.txt", O_WRONL | O_CREAT, 0);
	mov eax, 0x05      ; sys_open
	mov ebx, test_path ; Path
	mov ecx, 16        ; Length
	mov edx, 0         ; Mode
	int 0x80

	; write(fd, "Hello!", 6)
	mov ebx, eax       ; File Descriptor
	mov eax, 0x04      ; sys_write
	mov ecx, test_writ ; Data
	mov edx, 6         ; Length
	int 0x80

	add EAX, 1
	push EAX
	mov ECX, process1text
	push ECX
	call kprintf
	add ESP, 4
	pop EAX

jmp proc1ptl



times 4096 db 0




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



times 4096 db 0

process1text: db `Im process 1! Hello! Iteration: %d! `, 0
process2text: db 'Im process 2! Hello! Iteration: %d!', 0

test_path: db "abc/foo/test.txt", 0
test_writ: db "Hello!", 0


getprocess1:
	mov EAX, process1
ret


getprocess2:
	mov EAX, process2
ret
