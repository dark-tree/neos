
#include "types.h"
#include "console.h"
#include "print.h"

extern uint32_t test();
extern uint32_t kset(uint32_t base, uint32_t offset);

void start() {

	uint16_t volatile* screen = (uint16_t*) 0xB8000;
	kset(270*16, 1024*1024);
	
	con_init(80, 25);

	kprintf("\e[2J%% Hello \e[1;33m%s\e[m wo%cld, party like it's \e[1m0x%x\e[m again!", "sweet", 'r', 1920);

	while (true) {
		__asm("hlt");

	}
}
