
#include "types.h"


volatile bool truth = true;

extern uint32_t test();
extern uint32_t kset(uint32_t base, uint32_t offset);

void start() {
	uint16_t volatile* screen = (uint16_t*) 0xB8000;
	kset(270*16, 1024*1024);
	while (truth) {
		for (int i = 0; i < 320 * 200; i ++) {
			volatile uint8_t* chr = &screen[i];

			__asm ("nop");

			chr[0] = (char) kset(270*16, 1024*1024);
			chr[1] = 0b01001001;
		}
	}
}
