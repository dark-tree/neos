
#include "types.h"
#include "console.h"
#include "print.h"

extern char test();

#define X "\xDB"
#define S " "

void start() {
	con_init(80, 25);

	kprintf("\e[2J%% Hello \e[1;33m%s\e[m wo%cld, party like it's \e[1m0x%x\e[m again!\n", "sweet", 'r', 1920);

	kprintf("\e[4B");
	kprintf("\e[29C" X S S S X S X X X X S S X X X S S X X X X"\n");
	kprintf("\e[29C" X X S S X S S S S S S X S S S X S S S S S"\n");
	kprintf("\e[29C" X S X S X S X X S S S X S X S X S X X X X"\n");
	kprintf("\e[29C" X S S X X S S S S S S X S S S X S S S S X"\n");
	kprintf("\e[29C" X S S S X S X X X X S S X X X S S X X X X"\n");
	kprintf("\e[29C" " Linux Compatible OS\n");

	while (true) {
		__asm("hlt");
	}
}
