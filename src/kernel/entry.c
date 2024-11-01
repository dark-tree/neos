
#include "types.h"
#include "console.h"
#include "print.h"

extern uint32_t test();
extern void kset(uint32_t base, uint32_t offset);
extern uint32_t testtreesize(uint32_t level);
extern uint32_t testtreepointer(uint32_t level);
extern uint32_t kinternal_gettreeelement(uint32_t tree_level, uint32_t node_number);
extern void kinternal_settreeelement(uint32_t tree_level, uint32_t node_number, uint32_t value);
extern void kinternal_buddify(uint32_t tree_level, uint32_t node_number);
extern uint32_t  kinternal_allocate(uint32_t tree_level, uint32_t node_number, uint32_t segment_number_bitmask, uint32_t power_of_2);
extern void* kmalloc(uint32_t size);

void start() {

	kset(264*16-5, 1024*1024);

	con_init(80, 25);
	kprintf("\e[2J");
	for(int i=0;i<32;i++)
	{
		kprintf("%d %d,  ", testtreesize(i), testtreepointer(i));
	}
	kprintf("\n\n");

	
	//kinternal_settreeelement(2, 1, 0b111);
	//kinternal_settreeelement(1, 7, 0b00);
	//kinternal_settreeelement(1, 7, 0b11);
	//kinternal_buddify(1, 7);


	//kinternal_settreeelement(2, 3, 0b111);
	//kinternal_settreeelement(2, 2, 0b111);

	//kinternal_settreeelement(3, 1, 0b1111);
	//kinternal_settreeelement(3, 0, 0b1111);
	//kinternal_settreeelement(2, 1, 0b101);
	
	kmalloc(200);
	kmalloc(200);
	kmalloc(400);
	kmalloc(400);
	kmalloc(800);
	kmalloc(200);
	kmalloc(400);
	
	kprintf("%d\n", kinternal_allocate(4, 0,  0b100, 2));
	kprintf("%d\n", kinternal_gettreeelement(1, 7)); 
	kprintf("\n\n");
	kprintf("%d %d", *((uint8_t*)testtreepointer(0)), *((uint8_t*)(testtreepointer(0)+1)));
	kprintf("\n%d %d", *((uint8_t*)testtreepointer(1)), *((uint8_t*)(testtreepointer(1)+1)));
	kprintf("\n%d %d", *((uint8_t*)testtreepointer(2)), *((uint8_t*)(testtreepointer(2)+1)));
	kprintf("\n%d", *((uint8_t*)testtreepointer(3)));
	kprintf("\n%d", *((uint8_t*)testtreepointer(4)));



	while (true) {
		__asm("hlt");

	}
}
