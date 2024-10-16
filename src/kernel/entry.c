
#include "types.h"


volatile bool truth = true;

extern char test();

void start() {
	uint16_t volatile* screen = (uint16_t*) 0xB8000;

	while (truth) {
		for (int i = 0; i < 320 * 200; i ++) {
			volatile uint8_t* chr = &screen[i];

			__asm ("nop");

			chr[0] = test();
			chr[1] = 0b01001001;
		}
	}
}
