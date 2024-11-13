
cpu 386
bits 32

%define KMALLOC_BLOCK_SIZE 1024

section .text

extern memmove

global kmalloc
global kset
global kfree
global krealloc
global kmres

;This function reserves a designated memory area (takes it out of the allocation pool permamently)
;Takes 2 arguments: (void* pointer, uint32_t size)
kmres:
push EBP
mov EBP, ESP
push EBX
push ESI
push EDI
	mov EAX, [EBP+8]
	add EAX, [EBP+12]
	sub EAX, 1 ;Calculating where the last byte of the desired reservation is, by adding size to offset and subtracting 1

	sub EAX, [offset]
	mov EDX, 0
	mov EBX, KMALLOC_BLOCK_SIZE
	div EBX
	mov EDI, EAX	;Calculating the last segment of the reserved area (result in EDI)

	mov EAX, [EBP+8]
	sub EAX, [offset]
	mov EDX, 0
	div EBX
	mov ESI, EAX

	;Now the first segment of the reserved area is in ESI and the last one in EDI
	
	push DWORD 1
	push EDI
	push ESI
	push DWORD 0
	call areasettreeelement	;Marking segments as allocated
	add ESP, 16

	push EDI
	push ESI
	push DWORD 0
	call areafull_buddify	;Propagating the change up the tree using buddify
	add ESP, 12

pop EDI
pop ESI
pop EBX
pop EBP
ret




;This function takes an allocated memory area and makes it larger (Note: the area will be moved and the memory copied, if necesarry)
;Takes 2 arguments (void* pointer, uint32_t new_size), returns a pointer to the new area.
krealloc:

; We use ECX in place of EBP to make it debugable in GCC
mov ECX, ESP
sub ESP, 4

push EBP
push EBX
push ESI
push EDI

	; Load 'pointer'
	mov EAX, [ECX+4]
	sub EAX, [offset]
	mov [ECX+4], EAX


	; Convert pointer to segment number
	mov EDX, 0
	mov EBX, KMALLOC_BLOCK_SIZE
	div EBX
	mov EDI, EAX

	; Store segment number
	mov [ECX-4], EDI

	; Tree level
	xor ESI, ESI

	; Save stack for later
	push ECX

	jmp ra_ptl1_start

	; This loop determines how large is the allocated area currently (value will be stored in ESI as a power of 2 (2^ESI)
	ra_ptl1:
		inc ESI
		shr EDI, 1

	ra_ptl1_start:
		push EDI ; Element number (segment for level = 0)
		push ESI ; Tree level
		call gettreeelement
		add ESP, 8

	; Restart loop if element is empty (all bits equal to 0)
	test EAX, EAX
	jz ra_ptl1

	; We now have tree level of first non-zero element
	; "above" the given segment in ESI (0 based)

	; Restore our funky stack pointer
	pop ECX

	; Load 'new_size'
	mov EAX, [ECX+8]

	; ... and covert it to segments, and saves it in EAX
	sub EAX, 1
	xor EDX, EDX
	mov EBX, KMALLOC_BLOCK_SIZE
	div EBX
	add EAX, 1

	; Calculate greater or equal power of two
	xor EBX, EBX
	mov EDX, 1
	jmp ra_ptl2_start

	ra_ptl2:
		shl EDX, 1
		inc EBX

	ra_ptl2_start:
		cmp EDX, EAX
		jb ra_ptl2

	; EBX now contains the greater or equal to segment number power of two

	cmp ESI, EBX
	mov EAX, [ECX+4]
	jae ra_return

	add EAX, [offset]	;Adding offset, because we subtracted it earlier and kfree would subtract it again .

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
	
	push EAX
	push EBX
	call gettreeelement
	add ESP, 8

	test EAX, EAX
	pop EAX
	pop ECX
	jnz ra_move

	push ECX
	push EAX

	push DWORD 0xFFFFFFFF
	push EAX
	push EBX
	call settreeelement
	add ESP, 12
	
	pop EAX
	push EAX

	push EAX
	push EBX
	call full_buddify
	add ESP, 8

	pop EAX

	mov ECX, EBX

	shl EAX, CL

	pop ECX
	
	mov EDX, 0
	mov EBX, KMALLOC_BLOCK_SIZE
	mul EBX

	jmp ra_return
	ra_move:

	mov EAX, [ECX+8]
	push ECX

	push EAX
	call kmalloc
	add ESP, 4

	pop ECX

	test EAX, EAX
	jz ra_skip_adding	;Kmalloc returning 0 means, that allocation could not be completed, so we do not add offset to this value

	sub EAX, [offset]	;We are subtractring offset, because both kmalloc and krealloc add it at the end, so we would be adding it twice
	


	ra_return:
	

	push EAX

	push ECX
	mov ECX, ESI
	mov EDX, 1
	shl EDX, CL	;Size of the currently existing area in segments

	pop ECX

	mov EAX, EDX
	xor EDX, EDX
	mov EBX, KMALLOC_BLOCK_SIZE
	mul EBX

	mov EBX, EAX
	pop EAX
	push EAX

	add EAX, [offset]
	push EBX
	mov EBX, [ECX+4]
	add EBX, [offset]
	push EBX
	push EAX
	call memmove
	add ESP, 12

	pop EAX
	ra_skip_copying:

	
	

	add EAX, [offset]

	ra_skip_adding:
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
	sub EAX, [offset]
	mov EDX, 0
	mov EBX, KMALLOC_BLOCK_SIZE
	div EBX		;Determining which segment the pointer points to

	push EAX

	push DWORD 0
	push EAX
	push DWORD 0
	call settreeelement
	add ESP, 12

	pop EAX


	push EAX
	push DWORD 0
	call full_buddify
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
	call gettreeelement ;Reading the top tree node to check whether allocation is possible
	add ESP, 8

	pop ECX
	pop EDX

	;Checking if allocating an area of this size is possible
	and EAX, EBX
	mov EDI, 0
	cmp EDI, EAX
	mov EAX, 0
	jne m_skip_adding	;...if not, we're returning 0 (null)

	push ECX
	push EDX

	push ECX
	push EBX
	push DWORD 0
	push ESI
	call allocate	;Searching for a free tree node on tree level ECX
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
	call settreeelement
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
	call full_buddify
	add ESP, 8

	pop EDX
	pop ECX
	pop EAX

	shl EAX, CL	;We need to get the first descentant of node EAX on level ECX, that is on level 0, because that represents the first segment of the allocated memory area

	mov EDX, 0
	mov EBX, KMALLOC_BLOCK_SIZE
	mul EBX



	m_return:


	add EAX, [offset]

	m_skip_adding:
pop EDI
pop ESI
pop EBX
pop EBP
ret


;This fuction finds an empty spot in the control structure, starting from the given node and returns its segment number. It assumes that the area can be allocated in that node (you need to check for that before calling this fuction)
;Takes 4 arguments: (uint32_t tree_level, uint32_t node_number, uint32_t segment_number_bitmask, uint32_t which_power_of_two)
allocate:
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
	call gettreeelement
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
	call allocate
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
full_buddify:
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
	call buddify
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
buddify:
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
	call gettreeelement	;Moving value of the left child into EAX
	add ESP, 8
	pop EDX
	pop ECX

	mov EDX, EAX	;Moving value of the left child into EDX

	add EBX, 1	;Index of the right child

	push ECX
	push EDX
	push EBX
	push EBP
	call gettreeelement	;Moving value of the right child into EAX
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
	call settreeelement	;So we are saving this value into the tree
	add ESP, 12

pop EDI
pop ESI
pop EBX
pop EBP
ret





;Function takes 2 arguments: (uint32_t tree level, uint32_t node number)
;Returns an uint32_t, with the contains bits of that node (N youngest bits, where N-1 == level)
gettreeelement:
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


;Function applies buddify to multiple nodes on the same tree level
;Takes 3 arguments: (uint32_t tree_level, uint32_t start_node, uint32_t end_node) It will run buddify on every node from start_node to end_node (including those two)
levelbuddify:
mov EDX, ESP
push EBP
push EBX
push ESI
push EDI
	mov EBP, [EDX+4]	;Tree level, that we will be running this operation on is stored to EBP
	
	mov ESI, [EDX+8]
	mov EDI, [EDX+12]	;Start and end point

	lb_ptl1:
		push ESI
		push EBP
		call buddify
		add ESP, 8	;We are not restoring EAX, EDX and ECX, because we do not need those registers
		add ESI, 1	;Moving to the next node
	cmp ESI, EDI
	jbe lb_ptl1

pop EDI
pop ESI
pop EBX
pop EBP
ret


;This function calls full_buddify on a number of nodes in a time-effective manner.
;Takes 3 arguments: (uint32_t tree_level, uint32_t start_node, uint32_t end_node) It will run full_buddify on every node from start_node to end_node (including those two)

areafull_buddify:
mov EDX, ESP
push EBP
push EBX
push ESI
push EDI
	mov EBP, [EDX+4]	;Starting level (we will be incrementing this to move up the tree
	mov ESI, [EDX+8]
	mov EDI, [EDX+12]	;Starting and ending points

	mov EBX, [max_tree_level] ;Highest tree level - that's where we end

	afb_ptl1:
		;We start buddify from the level above:
		add EBP, 1 ;Moving level up
		shr ESI, 1	;Getting parent of the starting point node
		shr EDI, 1	;Getting parent of the ending point node

		push EDI	;Starting node number of this level
		push ESI	;Ending node number on this level
		push EBP	;Current level number
		call levelbuddify
		add ESP, 12

	cmp EBP, EBX
	jbe afb_ptl1 
pop EDI
pop ESI
pop EBX
pop EBP
ret


;This function sets multiple tree elements on a given level to the given value. 
;It takes 4 arguments: (uint32_t tree_level, uint32_t starting_node, uint32_t ending node, uint32_t value)
areasettreeelement:
mov EDX, ESP
push EBP
push EBX
push ESI
push EDI
	mov EBP, [EDX+4]	;Tree level
	mov EBX, [EDX+16]	;Node value
	mov ESI, [EDX+8]	;Start node
	mov EDI, [EDX+12] 	;End node

	aste_ptl1:
		push EBX
		push ESI
		push EBP
		call settreeelement
		add ESP, 12

		add ESI, 1	;Moving to the next node 
	cmp ESI, EDI
	jbe aste_ptl1

pop EDI
pop ESI
pop EBX
pop EBP
ret




;Function takes 3 arguments: (uint32_t tree_level, uint32_t node_number, uint32_t new_value)
;Note: only tree_level+1 youngest bits from the new_value will be used
settreeelement:
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




initializetree:
push EBP
push EBX
push ESI
push EDI

	mov EAX, [end]
	add EAX, [offset]
	mov [tree_levels+4], EAX



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


	;We initialize last X nodes in the lowest tree level with 1s (that's because those represent non-existent blocks, so we mark them as allocated from the start)
	mov EAX, [end]

	push EDX
	push EBX

	mov EDX, 0
	mov EBX, KMALLOC_BLOCK_SIZE
	div EBX	;Calculating how many free segments there will be, after taking into account size od the control structure

	pop EDX
	pop EBX

	mov ECX, [tree_levels]
	sub ECX, 1

	cmp EAX, ECX
	ja it_skiponesetting

	push EAX
	push ECX
	push EDX

	push DWORD 1
	push ECX
	push EAX
	push DWORD 0
	call areasettreeelement
	add ESP, 16

	pop EDX
	pop ECX
	pop EAX

	push DWORD ECX
	push DWORD EAX
	push DWORD 0
	call areafull_buddify
	add ESP, 12

	it_skiponesetting:


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

	mov EAX, [EBP+12]
	mov [offset], EAX

	mov EAX, [EBP+8]
	mov [size], EAX		;Save area size

	mov EBX, KMALLOC_BLOCK_SIZE
	mov EDX, 0
	div EBX	;Calculating how many blocks fit into memory
	mov [block_number], EAX

	;We need to calculate how many leaves the tree will have - this will be a power of 2, larger or equal than number of blocks
	mov ECX, 1
	mov EDX, [block_number]
	it_ptl1:
		shl ECX, 1
	cmp ECX, EDX
	jb it_ptl1
	mov [tree_levels], ECX	;We save the number of nodes, that the first tree level will have

	sub ECX, 1
	shr ECX, 1	;Calculating size od the control structute (4 bits per segment/block, so we divide the number of segments in 2)
	add ECX, 1

	mov EAX, [size]
	sub EAX, ECX
	mov [end], EAX

	;Initializing the tree (this function is taking parameters from this global variables)
	call initializetree
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
