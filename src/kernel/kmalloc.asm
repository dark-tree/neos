
cpu 386
bits 32

%define KMALLOC_BLOCK_SIZE 256
%define CONTROL_STRUCTURE_BYTES_PER_BLOCK 8

section .text

global test
global kset
global kmalloc

global testtreesize
global testtreepointer
global kinternal_gettreeelement
global kinternal_settreeelement
global kinternal_buddify
global kinternal_allocate
global kfree
global krealloc




;Thsi function takes an allocated memory area and makes it larger (Note: the area will be moved and the memory copied, if necesarry)
;Takes 2 arguments (void* pointer, uint32_t new_size), returns a pointer to the new area.
krealloc:
mov ECX, ESP
sub ESP, 4
push EBP
push EBX
push ESI
push EDI
	mov EAX, [ECX+4]
	mov EDX, 0
	mov EBX, KMALLOC_BLOCK_SIZE
	div EBX
	mov EDI, EAX	;Block number
	mov [ECX-4], EDI

	mov ESI, 0 ;Tree level
	
	push ECX

	jmp ra_ptl1_start

	ra_ptl1:	;THis loop determines how large is the allocated area currently (value will be stored in ESI as a power of 2 (2^ESI)
	add ESI, 1
	shr EDI, 1

	ra_ptl1_start:
	push EDI
	push ESI
	call kinternal_gettreeelement
	add ESP, 8

	mov EDX, 0
	cmp EAX, EDX
	je ra_ptl1

	pop ECX


	mov EAX, [ECX+8]
	mov EDX, 0
	mov EBX, KMALLOC_BLOCK_SIZE
	div EBX

	mov EBX, 0
	mov EDX, 1
	jmp ra_ptl2_start
	ra_ptl2:
	shl EDX, 1
	add EBX, 1
	ra_ptl2_start:
	cmp EDX, EAX
	jb ra_ptl2

	cmp ESI, EBX
	mov EAX, [ECX+4]
	jae ra_return

	mov EAX, [ECX+4]

	push ECX

	push EAX
	call kfree
	add ESP, 4

	pop ECX

	mov EAX, [ECX-4]

	push ECX

	mov ECX, EBX
	shr EAX, CL

	push EAX
	push EBX
	call kinternal_gettreeelement
	add ESP, 8

	pop ECX

	mov EDX, 0
	cmp EAX, EDX
	jne ra_move

	push ECX
	
	push EAX	;This is for the buddify call below

	push DWORD 0xFFFFFFFF
	push EAX
	push EBX
	call kinternal_settreeelement
	add ESP, 12

	;Here EAX is already pushed, so I only push the other argument
	push EBX
	call kinternal_full_buddify
	add ESP, 8

	pop ECX

	mov EAX, [ECX+4]

	jmp ra_return
	ra_move:

	mov EAX, [ECX+8]
	push ECX

	push EAX
	call kmalloc
	add ESP, 4

	pop ECX


	ra_return:
pop EDI
pop ESI
pop EBX
pop EBP
add ESP, 4
ret










;This function frees allocated memory area.
;Takes 1 argument: (void* pointer) - that pointer can be anywhere withing the area you want to free
kfree:
mov EDX, ESP
push EBP
push EBX
push ESI
push EDI
	mov EAX, [EDX+4]
	mov EDX, 0
	mov EBX, KMALLOC_BLOCK_SIZE
	div EBX		;Determining which segment the pointer points to

	push EAX

	push DWORD 0
	push EAX
	push DWORD 0
	call kinternal_settreeelement
	add ESP, 12

	pop EAX


	push EAX
	push DWORD 0
	call kinternal_full_buddify
	add ESP, 8



pop EDI
pop ESI
pop EBX
pop EBP
ret








;This function allocates a memory area of a given size
;Takes 1 argument: (uint32_t size)
kmalloc:
mov EDX, ESP
push EBP
push EBX
push ESI
push EDI
	mov EAX, [EDX+4]
	sub EAX, 1
	mov EDX, 0
	mov EBX, KMALLOC_BLOCK_SIZE
	div EBX
	add EAX, 1	;Now EAX stores the number of blocks that will need to be allocated

	;Due to how the allocator works, block number needs to be a power of 2, so we need to get a power of 2 greater or equal to EAX
	mov EDX, 1
	mov ECX, 0	;This number will inform us which power of 2 EDX is
	jmp skip_m_ptl1	;We want to check the condition before multiplying EDX by two, just in case EAX is equal to 1
	m_ptl1:
		shl EDX, 1	;Multiplying by 2
		add ECX, 1
	skip_m_ptl1:
	cmp EDX, EAX
	jb m_ptl1	;If EDX is less than EAX, jumping back

	;Now EDX stores the number of blocks we will actually allocate. ECX stores information on which power of 2 it is (we will need that information, because we need to know on which tree level in the control structure we need to look

	mov EBX, 1
	shl EBX, CL	;We are creating a bitmask that we will apply to tree nodes to check, whether allocating a block of consisting of 2^ECX == EDX number of segments is possible in that node.

	mov ESI, [max_tree_level]

	push EDX
	push ECX

	push DWORD 0
	push ESI
	call kinternal_gettreeelement ;Reading the top tree node to check whether allocation is possible
	add ESP, 8

	pop ECX
	pop EDX

	;Checking if allocating an area of this size is possible
	and EAX, EBX
	mov EDI, 0
	cmp EDI, EAX
	mov EAX, 0
	jne m_return	;...if not, we're returning 0 (null)

	push ECX
	push EDX

	push ECX
	push EBX
	push DWORD 0
	push ESI
	call kinternal_allocate	;Searching for a free tree node on tree level ECX
	add ESP, 16

	pop EDX
	pop ECX

	;Now EAX stores the number of node that we're allocating, on level number stored by ECX

	;Marking the area as allocated:
	push ECX
	push EDX
	push EAX

	push DWORD 0xFFFFFFFF
	push EAX
	push ECX
	call kinternal_settreeelement
	add ESP, 12

	pop EAX
	pop EDX
	pop ECX

	;Updating the tree upwards with a full buddify:
	push EAX
	push ECX
	push EDX

	push EAX
	push ECX
	call kinternal_full_buddify
	add ESP, 8

	pop EDX
	pop ECX
	pop EAX

	shl EAX, CL	;We need to get the first descentant of node EAX on level ECX, that is on level 0, because that represents the first segment of the allocated memory area

	mov EDX, 0
	mov EBX, KMALLOC_BLOCK_SIZE
	mul EBX


	m_return:
pop EDI
pop ESI
pop EBX
pop EBP
ret


;This fuction finds an empty spot in the control structure, starting from the given node and returns its segment number. It assumes that the area can be allocated in that node (you need to check for that before calling this fuction)
;Takes 4 arguments: (uint32_t tree_level, uint32_t node_number, uint32_t segment_number_bitmask, uint32_t which_power_of_two)
kinternal_allocate:
mov EDX, ESP
push EBP
push EBX
push ESI
push EDI
	mov ECX, [EDX+4] ;Tree level
	mov EBX, [EDX+8] ;Node number

	mov ESI, [EDX+16]
	mov EAX, EBX
	cmp ESI, ECX ;Checking if we've arrived on the proper tree level
	je a_return




	sub ECX, 1	;We will be checking children, so we move 1 level down
	shl EBX, 1	;We're checking left child first

	push ECX
	push EDX

	push EBX
	push ECX
	call kinternal_gettreeelement
	add ESP, 8

	pop EDX
	pop ECX

	mov ESI, [EDX+12]
	and EAX, ESI

	mov EDI, 0

	cmp EAX, EDI

	je a_call_recursively

	add EBX, 1 ;Moving onto the right child (we do not need to read that one, because we know for a fact that one of the children allows for allocation and we determined that the left one doesn't)



	a_call_recursively:

	push ECX
	push EDX

	mov EDI, [EDX+16]
	push EDI
	mov ESI, [EDX+12]
	push ESI
	push EBX
	push ECX
	call kinternal_allocate
	add ESP, 16

	pop EDX
	pop ECX

	a_return:
pop EDI
pop ESI
pop EBX
pop EBP
ret





;This function executes ,,buddify" on the given node and all its ancestors (parents, grandparents etc)
;Takes 2 arguments: (uint32_t tree_level, uint32_t node_number), returns nothing
kinternal_full_buddify:
mov EDX, ESP
push EBP
push EBX
push ESI
push EDI

mov EAX, [max_tree_level] ;Highest tree level
mov EBX, [EDX+4]	;Current tree level
mov ECX, [EDX+8]	;Current node number

;Loop calling buddify on all nodes above the selected one (notice, that we do not call buddify on thje given node, only it's ancestors - we do that because those function will be usiually code directly after updating the given node)..
fb_ptl1:
	add EBX, 1	;Moving 1 level up
	shr ECX, 1	;Parent of the current node


	push EAX
	push EDX
	push ECX
	;Arguments
	push ECX	;Node number
	push EBX	;Tree level
	call kinternal_buddify
	add ESP, 8

	pop ECX
	pop EDX
	pop EAX


cmp EBX, EAX
jb fb_ptl1

pop EDI
pop ESI
pop EBX
pop EBP
ret






;This function updates the node, so that it contains accurate information about block availibility. It does so, based on it's immediate children, so this function needs to be called on the entire tree branch at once, bottom to the top. 
;Takes 2 arguments: (uint32_t tree_level, uint32_t node_number), returns nothing
kinternal_buddify:
mov ECX, ESP
push EBP
push EBX
push ESI
push EDI
	mov EBP, [ECX+4]
	sub EBP, 1	;EBP contains level, where the dildren of this node are located
	mov EBX, [ECX+8]
	shl EBX, 1	;EBX containes index of the left child
	
	push ECX
	push EDX
	push EBX
	push EBP
	call kinternal_gettreeelement	;Moving value of the left child into EAX
	add ESP, 8
	pop EDX
	pop ECX

	mov EDX, EAX	;Moving value of the left child into EDX

	add EBX, 1	;Index of the right child

	push ECX
	push EDX
	push EBX
	push EBP
	call kinternal_gettreeelement	;Moving value of the right child into EAX
	add ESP, 8
	pop EDX
	pop ECX

	mov EBX, EAX
	and EBX, EDX	;and-ing values of both children - all bits of this node are determined this way, except for the oldest one (because if this node denotes 8-segment area, then allocating 4 segments is possible if either child has the space, but allocating 8 segments is only possible if both children are empty)

	mov ESI, [ECX+4]
	sub ESI, 1	;ESI contains the number of bits of each child minus 1 - so the number of bits we have to shift, so that olders bit of either child is on bit 0 in EAX and EDX registers
	mov EDI, ECX
	mov ECX, ESI
	shr EAX, CL
	shr EDX, CL
	or EAX, EDX	;If result of this is 0 - the the largest possible area (for example 8 segments in level-3 node can be allocated, because it means that both 4-segment level-2 nodes are free)
	shl EAX, CL
	shl EAX, 1	;Returning this bit to the appropriate spot (oldest bit of this node)
	

	or EBX, EAX	;Now EBX contains value this node should have

	mov EBP, [EDI+4]
	mov EAX, [EDI+8]

	push EBX
	push EAX
	push EBP
	call kinternal_settreeelement	;So we are saving this value into the tree
	add ESP, 12

pop EDI
pop ESI
pop EBX
pop EBP
ret





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

	mov EDI, [ESI+EBP]	;This whole block of code does a byte endian swap
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
	sub ECX, [EAX+8]
	sub ECX, 1
	shl EBX, CL	;Now our bitd are the X oldest bits in the double word (whis is basically input sanitization - if you try to input 6 bits into a 5-bit tree node, the olderst bit will be nullified)
	mov ECX, EDX
	shr EBX, CL
	mov [EAX+16], EBX	;Putting input bits at the right position in the entire dword to create a proper bitmask

	mov EBX, [EAX+8]
	add EBX, 1
	add EBX, EDX

	ste_ptl1:	;Setting corresponding bits to 0, so that we can easily set them to proper values using OR
		mov ECX, 31
		sub ECX, EDX
		btr EDI, ECX
	add EDX, 1
	cmp EDX, EBX
	jb ste_ptl1

	OR EDI, [EAX+16]

	mov EAX, EDI	;Another byte endian swap
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


	;Initialiing the entire tree with 0s (marking them as unallocated)
	mov EDI, [tree_levels+4]	;Table storing level 0 is the first, so it's pointer is also a pointer to the whole tree
	push EDX
	mov EDX, [size]
	add EDX, [offset]	;Calculating where control structure ends (at the end of the entire managed heap)

	it_ptl4:
		mov BYTE [EDI], 0
	add EDI, 1
	cmp EDI, EDX
	jb it_ptl4
	
	pop EDX
	
	
	push ECX

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
		;Descriptor of each tree level takes up 8 bytes
		mov [8*EAX + EBP], ECX	;Setting size of this tree level
		mov [8*EAX + EBP + 4], EDI	;Setting pointer to this tree level
		mov [max_tree_level], EAX	;Updating the information on what the current highest level is
		
		push EDX
		push EAX
		mov EDX, 0
		mov EAX, ECX
		mul EBX		;Bits per node multiplied by level size - how many bits does the current level take up
		sub EAX, 1
		shr EAX, 3	;How many bytes does the current level take up (dividing number of bits by 8, but first subtracting 1 and then adding 1, in order to round up the number)
		add EAX, 1	
		add EDI,EAX	;Calculating pointer to the next level, by taking pointer to the pravious level and adding it's size. 
		pop EAX
		pop EDX

		;Then we calculate the values for next level
		add EAX, 1	;New tree level
		add EBX, 1	;On new level each node will have 1 bit more, than on previous one
		shr ECX, 1	;Binary tree - each new level is half the size of the previous one

	push EAX
	mov EAX, 0
	cmp EAX, ECX	;We stop when size of the next level would be 0
	pop EAX
	jne it_ptl3


	pop ECX
	
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


		push EAX
		push EDX
		push ECX

		push EAX	;Calling full buddify on the node we just set to 1
		push DWORD 0	;...on level 0
		call kinternal_full_buddify
		add ESP, 8

		pop ECX
		pop EDX
		pop EAX

	add EAX, 1
	cmp EAX, ECX
	jb it_ptl2

	skip_it_ptl2:


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

max_tree_level: dd 0
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

