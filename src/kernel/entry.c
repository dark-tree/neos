
#include "types.h"
#include "console.h"
#include "ansi.h"

extern char test();

void start() {
	con_init(80, 25);

	con_write('B');
	con_write('y');
	con_write('e');
	con_write('?');
	con_write(ANSI_BACKSPACE);
	con_write('!');
	con_write(ANSI_LINE_FEED);

	con_write('1');
	con_write('2');
	con_write('3');
	con_write(ANSI_CARRIAGE_RETURN);
	con_write('+');

	con_write(ANSI_TAB);
	con_write('4');
	con_write('5');

	con_write(ANSI_TAB);
	con_write('6');
	con_write('7');

	con_write(ANSI_TAB);
	con_write('8');
	con_write('9');


	while (true) {
		__asm("hlt");
	}
}
