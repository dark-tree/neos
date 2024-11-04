#include "interrupt.h"
#include "print.h"
#include "config.h"
#include "types.h"

// https://wiki.osdev.org/Interrupt_Descriptor_Table
typedef struct {
	uint16_t offset_1;   // Offset bits 0..15
	uint16_t selector;   // Code segment selector
	uint8_t  unused;     // Unused and ignored
	uint8_t  attributes; // Gate type, DPL, and P fields
	uint16_t offset_2;   // Offset bits 16..31
} ABI_PACKED InterruptDescriptor;

// defined in routine.asm
extern void isr_init();
extern void isr_register(int number, interupt_hander handler);
extern void idtr_store(int offset, int limit);
extern void gdtr_switch(int data, int code);

void idt_await(int interupt) {

}

void idt_test_function(int number, void* user) {

}

void test_int(int number, int error) {
	kprintf("Interupt: 0x%x, err=%d\n", number, error);

	if (number != 8) {
		while (true) {
			__asm("hlt");
		}
	}
}

void idt_init() {

	isr_init();

	kprintf("UwU!\n", sizeof(InterruptDescriptor));

	for (int i = 0; i < 0x81; i ++) {
		isr_register(i, test_int);
	}

	kprintf("Owo!\n");


	idtr_store(0, 0x81);
	//gdtr_switch(2, 1);

}
