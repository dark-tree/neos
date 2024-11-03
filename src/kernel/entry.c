
#include "types.h"
#include "console.h"
#include "floppy.h"
#include "print.h"

void start() {
	con_init(80, 25);

	//kprintf("\e[2J%% Hello \e[1;33m%s\e[m wo%cld, party like it's \e[1m0x%x\e[m again!\n", "sweet", 'r', 1920);

	if(floppy_init()){
		uint8_t buffer[2048];
		if(floppy_read(buffer, 0xa200, 1400))

		for (int i = 0; i < 1400; i++) {
			kprintf("%c", buffer[i]);
		}
		kprintf("\n");
	}
	else{
		kprintf("Error: Floppy not initialized\n");
	}

	while (true) {
		__asm("hlt");
	}
}
