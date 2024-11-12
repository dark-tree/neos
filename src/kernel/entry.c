
#include "types.h"
#include "console.h"
#include "print.h"

extern void kset(uint32_t base, uint32_t offset);
extern void* kmalloc(uint32_t size);
extern void kfree(void* pointer);
extern void kmres(void* pointer, uint32_t size);
extern void* krealloc(void* pointer, uint32_t new_size);

void start() {
        con_init(80, 25);
	kprintf("\e[2J");


	kset(1024*16, 1024*1024);

	kmres(1024*1024+2048, 1027);


	void * ptr5 = kmalloc(200);
	kmalloc(100);
	void * ptr6 = kmalloc(1599);
	void * ptr7 = kmalloc(700);

	kprintf("%d\n", (int) ptr5);
	kprintf("%d\n", (int) ptr6);
	kprintf("%d\n", (int) ptr7);


	kfree(ptr6);

	ptr6 = kmalloc(500);
	void * ptr8 = kmalloc(700);
	kprintf("%d\n", (int) ptr6);
	kprintf("%d\n", (int) ptr8);

	ptr8 = krealloc(ptr8, 3000);
	kprintf("%d\n", (int) ptr8);
	

	while (true) {
		__asm("hlt");

	}
}


