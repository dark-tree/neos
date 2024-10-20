
#include "types.h"
#include "console.h"
#include "ansi.h"

extern char test();

void start() {
	con_init(80, 25);

	for (int i = 0; i < 7; i ++) {
		con_write('0');
		con_write('1');
		con_write('2');
		con_write('3');
	}

	con_write(ANSI_LINE_FEED);

	con_write('1');
	con_write('2');
	con_write('3');
	con_write(ANSI_CARRIAGE_RETURN);
	con_write('+');

	con_write(ANSI_ESCAPE);
	con_write(ANSI_CSI);
	con_write('9');
	con_write('4');
	con_write(';');
	con_write('1');
	con_write('0');
	con_write('1');
	con_write(ANSI_CSI_SGR);

	con_write(ANSI_TAB);
	con_write('4');
	con_write('5');

	con_write(ANSI_ESCAPE);
	con_write(ANSI_CSI);
	con_write('7');
	con_write(ANSI_CSI_SGR);

	con_write(ANSI_TAB);
	con_write('4');
	con_write('5');

	con_write(ANSI_ESCAPE);
	con_write(ANSI_CSI);
	con_write('2');
	con_write('7');
	con_write(ANSI_CSI_SGR);

	con_write(ANSI_TAB);
	con_write('4');
	con_write('5');

	con_write(ANSI_ESCAPE);
	con_write(ANSI_CSI);
	con_write('3');
	con_write('9');
	con_write(ANSI_CSI_SGR);

	con_write(ANSI_TAB);
	con_write('6');
	con_write('7');

	con_write(ANSI_ESCAPE);
	con_write(ANSI_CSI);
	con_write('4');
	con_write('9');
	con_write(ANSI_CSI_SGR);

	con_write(ANSI_TAB);
	con_write('8');
	con_write('9');

	con_write(ANSI_ESCAPE);
	con_write(ANSI_CSI);
	con_write(ANSI_CSI_SGR);

	con_write(ANSI_TAB);
	con_write('a');
	con_write('b');

	con_write(ANSI_ESCAPE);
	con_write(ANSI_CSI);
	con_write('9');
	con_write(';');
	con_write('1');
	con_write(ANSI_CSI_HVP);

	con_write('X');


	con_write(ANSI_ESCAPE);
	con_write(ANSI_CSI);
	con_write('0');
	con_write(ANSI_CSI_EL);

//	con_write(ANSI_ESCAPE);
//	con_write(ANSI_CSI);
//	con_write('2');
//	con_write(ANSI_CSI_SD);

	while (true) {
		__asm("hlt");
	}
}
