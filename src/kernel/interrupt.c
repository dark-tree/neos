#include "interrupt.h"

void idt_register(int interupt, interupt_hander handler, void* user) {
	handler(interupt, user);
}

void idt_await(int interupt) {

}

void idt_test_function(int number, void* user) {
	kprintf("Interupt: %d, usr='%s'\n", number, user);
}
