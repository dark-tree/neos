
cpu 386
bits 32

%define KMALLOC_BLOCK_SIZE 256
%define CONTROL_STRUCTURE_BYTES_PER_BLOCK 8

section .text

global test
global kset


;Function takes 2 arguments: (uint32_t tree level, uint32_t node number)
;Returns an uint32_t, with the contains bits of that node (N youngest bits, where N-1 == level) 
kinternal_gettreeelement:
push EBP
mov EBP, ESP
push EBX
push ESI
push EDI
	mov ESI, [EBP+8]
	add ESI,  tree_levels	;All levels are tables, ESI register will contain a pointer to the one we need 

	mov EBX, [EBP+8]
	add EBX, 1	;All elements of level 0 are 1-bit, elements of level 1 are 2-bit etc. Hence we're adding 1

	mov EDX, 0
	mov EAX, [EBP+12]
	div EBX			;Calculating which bit in which byte of the array our bits start at

	mov EAX, [ESI+EAX]
	mov ECX, EDX
	shl EAX, CL	;Bits we needed were somewhere in the middle of the EAX register, now those are the oldes bits

	mov EBX, 32
	mov EBX, EDX	;Calculating how much we need to shift right, for our bits to be N youngest bits of that array

	mov ECX, EBX
	shr EAX, CL
pop EDI
pop ESI
pop EBX
pop EBP
ret


;Function takes 3 arguments: (uint32_t tree_level, uint32_t node_number, uint32_t new_value)
;Note: only tree_level+1 youngest bits from the new_value will be used 
kinternal_settreeelement:
push EBP
mov EBP, ESP
push EBX
push ESI
push EDI
	mov ESI, [EBP+8]
	add ESI,  tree_levels	;All levels are tables, ESI register will contain a pointer to the one we need 

	mov EBX, [EBP+8]
	add EBX, 1	;All elements of level 0 are 1-bit, elements of level 1 are 2-bit etc. Hence we're adding 1

	mov EDX, 0
	mov EAX, [EBP+12]
	div EBX			;Calculating which bit in which byte of the array our bits start at

	mov EDI, [ESI+EAX]


	mov EBX, [EBP+16]
	shl EBX, 32
	mov ECX, EDX
	shr EBX, CL
	mov [EBP+16], EBX	;Putting input bits at the right position in the entire dword to create a proper bitmask

	mov EBX, [EBP+12]
	add EBX, EDX

	ste_ptl1:	;Setting corresponding bits to 0, so that we can easily set them to proper values using OR
		btr EDI, EDX
	add EDX, 1
	cmp EDX, EBX
	jb ste_ptl1

	OR EDI, [EBP+16]
	mov [ESI+EAX], EDI

pop EDI
pop ESI
pop EBX
pop EBP
ret


test2:
	push EBP
	mov EBP, ESP
	push EBX
	push ESI
	push EDI
	mov EAX, 'J'
	pop EDI
	pop ESI
	pop EBX
	pop EBP
	ret



kinternal_initializetree:
push EBP
mov EBP, ESP
push EBX
push ESI
push EDI
	mov EAX, [end]
	mov [tree_levels+4], EAX

	;First, we need to calculate how many leaves the tree will have - this will be a power of 2, larger or equal than number of blocks
	mov ECX, 1
	mov EDX, [block_number]
	it_ptl1:
		shl ECX, 1
	cmp ECX, EDX
	jb it_ptl1
	mov [tree_levels], ECX	;We save the number of nodes, that the first tree level will have

	;We initialize last X nodes in the lowest tree level with 1s (that's because those represent non-existent blocks, so we mark them as allocated from the start)
	mov EAX, [block_number]
	it_ptl2:
		push EAX
		push EDX
		push ECX
		push DWORD 1	;Setting node to 1
		push EAX	;Node number EAX
		push DWORD 0	;On tree level 0
		call kinternal_settreeelement
		add ESP, 12
		pop ECX
		pop EDX
		pop EAX
	add EAX, 1
	cmp EAX, ECX
	jb it_ptl2
	
	shr EDX, 3 	;On level 0 each node has 1 bit, so we divide by 8 to get total array size

	mov EAX, [end]
	add EDI, EAX	;From this byte we start allocating next levels, starting from 1

	;Now we are allocating memory for all tree levels:
	mov ECX, [tree_levels]	
	shr ECX, 1	;Node number on level 1 (twice less than level 0 - its a binary tree)
	mov EAX, 1	;Level number
	mov EBX, 2	;Bits per node on this level
	it_ptl3:
		;First we set the descriptor for the current level
		push ECX
		mov ECX, tree_levels
		push EAX
		shl EAX, 3	;Descriptor of each tree level takes up 8 bytes
		mov [EAX + ECX], ECX
		mov [EAX + ECX + 4], EDI
		pop EAX
		pop ECX

		;Then we calculate the values for next level
		add EAX, 1
		add EBX, 1
		shr ECX, 1

		push EDX
		push EAX
		mov EDX, 0
		mov EAX, ECX
		div EBX
		add EAX, 1
		add EDI,EAX
		pop EAX
		pop EDX
	push EAX
	mov EAX, 0
	cmp EAX, ECX
	pop EAX
	jne it_ptl3
	


pop EDI
pop ESI
pop EBX
pop EBP
ret






;Setting the boundaries of the memory domain managed by this allocator
;Taking 2 arguments: uint32_t size,  void* offset
kset:
push EBP
mov EBP, ESP
push EBX
	mov EAX, [EBP+8]
	mov [size], EAX		;Save area size

	mov EBX, KMALLOC_BLOCK_SIZE
	add EBX, CONTROL_STRUCTURE_BYTES_PER_BLOCK	;Adding, to take into account X bytes of control structure per block
	mov EDX, 0
	div EBX	;Calculating how many blocks fit into memory
	mov [block_number], EAX
	mov EBX, CONTROL_STRUCTURE_BYTES_PER_BLOCK
	mov EDX, 0
	mul EBX
	mov EBX, [size]	;Calculating the size of the control structure 
	sub EBX, EAX	;Calculating offset of the control structure0x8
	mov [end], EBX		;At the end of the managed domain there will be information about reserved blocks stored, this number represents where it starts.
	mov EAX, [EBP+12]
	mov [offset], EAX	;Save area offset
	mov EAX, 'H'
	;Initializing the tree (this function is taking parameters from this global variables)
	call test2
pop EBX
pop EBP
ret






;Function for test purposes - retrieving things from memory.
test:
	mov EAX, KMALLOC_BLOCK_SIZE
ret











section .data

end: dd 0
size: dd 0
offset: dd 0
block_number: dd 0

tree_levels: times 64 dd 0 
;Pointers to the tree structure controlling memory allocation
;Structure:
;	4 bytes		|	4 bytes
;	________________|_________________
;	tree lvl 0 size	|tree lvl 0 pointer
;	tree lvl 1 size	|tree lvl 1 pointer
;	...		|...
;
;	Levels are numbered from the lowest (level 0 is leaves)

