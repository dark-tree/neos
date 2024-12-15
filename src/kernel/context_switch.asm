cpu 386
bits 32

section .text

extern scheduler_context_switch
extern isr_into_stack
extern dump

global context_switch

context_switch:
	mov EAX, ESP
	add EAX, 4
	push EAX
	call scheduler_context_switch
	add ESP, 4
push EAX
call isr_into_stack
