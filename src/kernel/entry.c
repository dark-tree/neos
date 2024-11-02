
#include "types.h"
#include "console.h"
#include "floppy.h"
#include "print.h"

void start() {
	con_init(80, 25);

	kprintf("\e[2J%% Hello \e[1;33m%s\e[m wo%cld, party like it's \e[1m0x%x\e[m again!\n", "sweet", 'r', 1920);

	floppy_init();

	while (true) {
		__asm("hlt");
	}
}
