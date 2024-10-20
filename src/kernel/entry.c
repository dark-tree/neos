
#include "types.h"
#include "console.h"
#include "print.h"

extern char test();

void start() {
	con_init(80, 25);

	kprintf("%% Hello \e[1;33m%s\e[m wo%cld, party like it's \e[1m0x%x\e[m again!", "sweet", 'r', 1920);

	while (true) {
		__asm("hlt");
	}
}
