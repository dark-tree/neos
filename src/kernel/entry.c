
#include "types.h"


volatile bool truth = true;

extern uint32_t test();
extern void kset(uint32_t base, uint32_t offset);

void start() {
	uint16_t volatile* screen = (uint16_t*) 0xB8000;
	kset('K', 'J');
	while (truth) {
		for (int i = 0; i < 320 * 200; i ++) {
			volatile uint8_t* chr = &screen[i];

			__asm ("nop");

			chr[0] = (char)test();
			chr[1] = 0b01001001;
		}
	}
}
