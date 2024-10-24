
cpu 386
bits 32

%define KMALLOC_BLOCK_SIZE 256
%define CONTROL_STRUCTURE_BYTES_PER_BLOCK 8

section .text

global test
global kset
global testtreesize
global testtreepointer
global kinternal_gettreeelement
global kinternal_settreeelement

;Function takes 2 arguments: (uint32_t tree level, uint32_t node number)
;Returns an uint32_t, with the contains bits of that node (N youngest bits, where N-1 == level) 
kinternal_gettreeelement:
push EBP
mov EBP, ESP
push EBX
push ESI
push EDI
	mov EBX, [EBP+12]
	mov EDX, 0
	mov EAX, [EBP+8]
	add EAX, 1	;Node size = level_bumber + 1
	mul EBX
	mov [EBP+12], EAX

	mov ESI, [EBP+8]
	mov EBX, tree_levels
	mov ESI,  [EBX+8*ESI+4]	;All levels are tables, ESI register will contain a pointer to the one we need 

	mov EBX, 8
	mov EDX, 0
	mov EAX, [EBP+12]
	div EBX			;Calculating from  which bit in which byte of the array our bits start at, dividing bit number by byte size

	mov EAX, [ESI+EAX]
	movzx EBX, AL
	shl EBX, 8
	shr EAX, 8
	mov BL, AL
	shl EBX, 8
	shr EAX, 8
	mov BL, AL
	shl EBX, 8
	shr EAX, 8
	mov BL, AL
	mov EAX, EBX
	
	mov ECX, EDX
	shl EAX, CL	;Bits we needed were somewhere in the middle of the EAX register, now those are the oldes bits

	mov EBX, 32
	sub EBX, [EBP+8]
	sub EBX, 1	;Calculating how much we need to shift right, for our bits to be N youngest bits of that array

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
mov EAX, ESP
push EBX
push ESI
push EDI
	;Multiplying node size by number of bits in a node, to get bit number.
	push EAX
	mov EBX, [EAX+12]
	mov EDX, 0
	mov EAX, [EAX+8]
	add EAX, 1	;Node size = level_bumber + 1
	mul EBX
	mov EBX, EAX
	pop EAX
	mov [EAX+12], EBX

	mov ESI, [EAX+8]
	mov EBX, tree_levels
	mov ESI, [EBX+8*ESI+4]	;All levels are tables, ESI register will contain a pointer to the one we need 

	mov EBX, 8
	mov EDX, 0
	mov EBP, [EAX+12]
	push EAX
	mov EAX, EBP
	div EBX
	mov EBP, EAX
	pop EAX		;Calculating from which bit in which byte of the array our bits start at, by diding bit number by byte size.

	mov EDI, [ESI+EBP]
	push EAX
	push EBX
	mov EAX, EDI
	movzx EBX, AL
	shl EBX, 8
	shr EAX, 8
	mov BL, AL
	shl EBX, 8
	shr EAX, 8
	mov BL, AL
	shl EBX, 8
	shr EAX, 8
	mov BL, AL
	mov EDI, EBX
	pop EBX
	pop EAX

	mov EBX, [EAX+16]
	mov ECX, 32
	sub ECX, EDX
	sub ECX, [EAX+8]
	sub ECX, 1
	shl EBX, CL
	mov [EAX+16], EBX	;Putting input bits at the right position in the entire dword to create a proper bitmask

	mov EBX, [EAX+12]
	add EBX, EDX

	ste_ptl1:	;Setting corresponding bits to 0, so that we can easily set them to proper values using OR
		btr EDI, EDX
	add EDX, 1
	cmp EDX, EBX
	jb ste_ptl1

	OR EDI, [EAX+16]

	mov EAX, EDI
	movzx EBX, AL
	shr EAX, 8
	shl EBX, 8
	mov BL, AL
	shr EAX, 8
	shl EBX, 8
	mov BL, AL
	shr EAX, 8
	shl EBX, 8
	mov BL, AL
	mov [ESI+EBP], EBX

pop EDI
pop ESI
pop EBX
pop EBP
ret




testtreesize:
	mov EAX, [ESP+4]
	mov EAX, [tree_levels+8*EAX]
	ret



testtreepointer:
	mov EAX, [ESP+4]
	mov EDX, tree_levels
	mov EAX, [EDX+8*EAX+4]
	ret




kinternal_initializetree:
push EBP
push EBX
push ESI
push EDI

	mov EAX, [end]
	add EAX, [offset]
	mov [tree_levels+4], EAX

	;First, we need to calculate how many leaves the tree will have - this will be a power of 2, larger or equal than number of blocks
	mov ECX, 1
	mov EDX, [block_number]
	it_ptl1:
		shl ECX, 1
	cmp ECX, EDX
	jb it_ptl1
	mov [tree_levels], ECX	;We save the number of nodes, that the first tree level will have


	;Initialiing the entire tree level 0 with 0s (marking them as unallocated), higher tree levels will be initialized recursively based on the first one.
	mov EDI, [tree_levels+4]
	mov EAX, 0
	push EDX
	mov EDX, ECX
	shr EDX, 3

	it_ptl4:
		mov BYTE [EAX+EDI], 0
	add EAX, 1
	cmp EAX, EDX
	jb it_ptl4
	
	pop EDX



	;We initialize last X nodes in the lowest tree level with 1s (that's because those represent non-existent blocks, so we mark them as allocated from the start)
	mov EAX, [block_number]
	cmp EAX, ECX
	jae skip_it_ptl2
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

	skip_it_ptl2:
	
	mov EDI, [tree_levels]	;Size of the array represening level 0
	shr EDI, 3 	;On level 0 each node has 1 bit, so we divide by 8 to get total array size
	mov EAX, [end]
	add EDI, EAX	;From this byte we start allocating next levels, starting from 1
	add EDI, [offset]

	;Now we are allocating memory for all tree levels:
	mov ECX, [tree_levels]	
	shr ECX, 1	;Node number on level 1 (twice less than level 0 - its a binary tree)
	mov EAX, 1	;Level number
	mov EBX, 2	;Bits per node on this level
	it_ptl3:
		;First we set the descriptor for the current level
		
		mov EBP, tree_levels
		push EAX
		shl EAX, 3	;Descriptor of each tree level takes up 8 bytes
		mov [EAX + EBP], ECX
		mov [EAX + EBP + 4], EDI
		pop EAX
		

		;Then we calculate the values for next level
		add EAX, 1
		add EBX, 1
		shr ECX, 1

		push EDX
		push EAX
		mov EDX, 0
		mov EAX, ECX
		mul EBX
		shr EAX, 3
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
	

	;Initializing the tree (this function is taking parameters from this global variables)
	call kinternal_initializetree
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

