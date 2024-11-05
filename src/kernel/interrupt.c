#include "interrupt.h"
#include "print.h"
#include "config.h"
#include "types.h"
#include "tables.h"
#include "routine.h"
#include "util.h"
#include "pic.h"

void test_int(int number, int error) {
	kprintf("Interupt: 0x%x, err=%d\n", number, error);
	//halt();
}

void idt_init() {

	// Fill the IDT will valid gate descriptors
	isr_init(MEMORY_MAP_IDT);

	for (int i = 0; i < 0x81; i ++) {
		isr_register(i, test_int);
	}

	//pic_disable();

	// Point the processor at the IDT and enable interrupts
	idtr_store(MEMORY_MAP_IDT, 0x81);

	// Enable hardware interrupts
	pic_enable();

}
