
#include "types.h"
#include "console.h"
#include "print.h"

extern uint32_t test();
extern void kset(uint32_t base, uint32_t offset);
extern uint32_t testtreesize(uint32_t level);
extern uint32_t testtreepointer(uint32_t level);

void start() {

	kset(264*16-5, 1024*1024);

	con_init(80, 25);
	kprintf("\e[2J");
	for(int i=0;i<32;i++)
	{
		kprintf("%d %d,  ", testtreesize(i), testtreepointer(i));
	}
	kprintf("\n\n");
	kprintf("%d %d", *((uint8_t*)testtreepointer(0)), *((uint8_t*)(testtreepointer(0)+1)));
	while (true) {
		__asm("hlt");

	}
}
