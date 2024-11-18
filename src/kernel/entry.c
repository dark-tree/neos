
#include "types.h"
#include "console.h"
#include "print.h"
#include "interrupt.h"
#include "util.h"
#include "cursor.h"
#include "mem.h"

extern char asm_test();
extern void pic_disable();

#define X "\xDB"
#define S " "

void start() {

	con_init(80, 25);
	cur_enable();

	// Init memory system and make room for the kernel
	mem_init(0xFFFFF);

    scheduler_init();

    scheduler_new_entry(14, 0, 0);
    scheduler_new_entry(12, 0, 0);
    scheduler_new_entry(10, 0, 0);
    scheduler_new_entry(8, 0, 0);
    scheduler_new_entry(6, 0, 0);

    kprintf("\n%d", get_index(0));
    kprintf("\n%d", get_index(1));
    kprintf("\n%d", get_index(2));
    kprintf("\n%d", get_index(3));
    kprintf("\n%d", get_index(4));
    halt();

//	kprintf("\e[2J%% Hello \e[1;33m%s\e[m wo%cld, party like it's \e[1m%#0.8x\e[m again!\n", "sweet", 'r', -1920);

//	kprintf("\e[4B");
//	kprintf("\e[29C" X S S S X S X X X X S S X X X S S X X X X"\n");
//	kprintf("\e[29C" X X S S X S S S S S S X S S S X S S S S S"\n");
//	kprintf("\e[29C" X S X S X S X X S S S X S X S X S X X X X"\n");
//	kprintf("\e[29C" X S S X X S S S S S S X S S S X S S S S X"\n");
//	kprintf("\e[29C" X S S S X S X X X X S S X X X S S X X X X"\n");
//	kprintf("\e[29C" " Linux Compatible OS\n");

	pic_disable();
	int_init();

	kprintf("System ready!\n");

	//asm_test();

	// never return to the bootloader
	halt();
}


