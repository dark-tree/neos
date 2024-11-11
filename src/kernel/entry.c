
#include "types.h"
#include "console.h"
#include "print.h"
#include "interrupt.h"
#include "util.h"
#include "cursor.h"
#include "config.h"

extern char asm_test();
extern void pic_disable();

#define X "\xDB"
#define S " "

typedef struct {
	uint64_t base;
	uint64_t length;
	uint32_t type;
} ABI_PACKED MemoryEntry;

void start() {
	con_init(80, 25);
	cur_enable();

	// display the memory map
	{
		int length = *((uint32_t*) MEMORY_MAP_RAM);

		kprintf("Found %d memory map entries\n", length);

		for (int i = 0; i < length; i ++) {
			MemoryEntry* entry = ((void*) MEMORY_MAP_RAM) + i * 20 + 4;

			const char* type_str = "Free";

			if (entry->type != 1) {
				type_str = "Reserved";
			}

			kprintf(" * %#0.8x (%#0.8x bytes) type: %s\n", (int) entry->base, (int) entry->length, type_str);
		}
	}

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
