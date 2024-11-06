#include "interrupt.h"
#include "print.h"
#include "config.h"
#include "types.h"
#include "tables.h"
#include "routine.h"
#include "util.h"
#include "pic.h"
#include "syscall.h"

/* private */

// Used by the wait subsystem
static int key;
static int locked;

// You can use this handle to debug the incoming interrupts
static void int_debug_handle(int number, int error, int* eax, int ecx, int edx, int ebx, int esi, int edi) {
	kprintf("Interrupt: 0x%x (%s), err=%d\n", number, isr_name(number), error);
	kprintf(" * EAX=0x%x, EBX=0x%x\n", *eax, ebx);
	kprintf(" * ECX=0x%x, EDX=0x%x\n", ecx, edx);
	kprintf(" * ESI=0x%x, EDI=0x%x\n", esi, edi);
}

// Forward syscalls to the syscall system
static void int_linux_handle(int number, int error, int* eax, int ecx, int edx, int ebx, int esi, int edi) {

	// write the return value, this change be visible after 'iret' ('int 0x80' in the calling code)
	*eax = sys_linux(*eax, ebx, ecx, edx, esi, edi);

}

/* public */

// This handle is invoked from assembly for all valid interrupts, please do not add any code here nor make it static
void int_common_handle(int number) {
	if (locked && (number == key)) {
		locked = false;
	}
}

void int_lock(int interrupt) {
	key = interrupt;
	locked = true;
}

void int_wait() {
	wait:
	__asm volatile ("hlt");

	if (locked) {

		// Why is this written like assembly??
		// The reasoning is left as an exercise for the reader!
		goto wait;

	}
}

void int_init() {

	// init the wait subsystem
	locked = false;
	key = 0xFF;

	// Fill the IDT will valid gate descriptors
	isr_init(MEMORY_MAP_IDT);

	// register interrupt handlers
	for (int i = 0; i <= 0x80; i ++) {
		isr_register(i, int_debug_handle);
	}

	isr_register(0x20, NULL);             // stop the timer spam
	isr_register(0x80, int_linux_handle); // forward syscalls to the syscall system

	// Point the processor at the IDT and enable interrupts
	idtr_store(MEMORY_MAP_IDT, 0x81);

	// Enable hardware interrupts
	pic_enable();

}
