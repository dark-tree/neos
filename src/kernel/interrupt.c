#include "interrupt.h"
#include "print.h"
#include "config.h"
#include "types.h"

extern void isr_test();

// P=1, DPL=3, Type=Gate
#define IDT_DEFAULT_GATE 0xEE

// https://wiki.osdev.org/Interrupt_Descriptor_Table
typedef struct {
	uint16_t offset_1;   // Offset bits 0..15
	uint16_t selector;   // Code segment selector
	uint8_t  unused;     // Unused and ignored
	uint8_t  attributes; // Gate type, DPL, and P fields
	uint16_t offset_2;   // Offset bits 16..31
} ABI_PACKED InterruptDescriptor;

void idt_write(int index, void* offset, int gdt_index) {
	InterruptDescriptor* interrupt = ((InterruptDescriptor*) MEMORY_MAP_IDT) + index;

	interrupt->selector = (gdt_index << 3);
	interrupt->attributes = IDT_DEFAULT_GATE;
	interrupt->unused = 0;
	interrupt->offset_1 = (((uint32_t) offset) & 0x0000FFFF);
	interrupt->offset_2 = (((uint32_t) offset) & 0xFFFF0000) >> 16;
}


extern void idtr_store(int offset, int limit);
extern void gdtr_switch(int data, int code);

void idt_register(int interupt, interupt_hander handler, void* user) {
	handler(interupt, user);
}

void idt_await(int interupt) {

}

void idt_test_function(int number, void* user) {
	kprintf("Interupt: %d, usr='%s'\n", number, user);
}

void idt_init() {

	for (int i = 0; i < 255; i ++) {
		idt_write(i, isr_test, 1);
	}

	idtr_store(MEMORY_MAP_IDT, 256);



	gdtr_switch(2, 1);
	idt_register(0, idt_test_function, "Test");
}
