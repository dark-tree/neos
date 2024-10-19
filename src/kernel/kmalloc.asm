
cpu 386
bits 32


section .text

global test
global kset

;Setting the boundaries of the memory area managed by this allocator
;Taking 2 arguments: uint32_t size,  void* offset
kset:
	mov EAX, [ESP+4]
	mov [size], EAX
	sub EAX, 1
	mov [end], EAX
	mov EAX, [ESP+8]
	mov [offset], EAX
	ret






;Function for test purposes - retrieving things from memory.
test:
	mov EAX, [offset]
	ret



















section .data

end: dd 0
size: dd 0
offset: dd 0
