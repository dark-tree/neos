#include "console.h"
#include "memory.h"
#include "ansi.h"

/* private */

#define ATTR_DEFAULT 0b00000111;

static uint8_t attribute;
static int width, height;
static int x, y;

static uint8_t* con_at(int x, int y) {
	return (uint8_t*) &((uint16_t*) 0xB8000)[x + y * width];
}

/* public */

void con_init(int max_width, int max_height) {
	width = max_width;
	height = max_height;
	x = 0;
	y = 0;
	attribute = ATTR_DEFAULT;
}

void con_scroll(int offset) {
	long row = width * sizeof(uint16_t);
	long last = height - 1;

	if (offset == -1) {
		memmove(con_at(0, 1), con_at(0, 0), last * row);
		memset(con_at(0, 0), 0, row);

		y ++;
		if (y > last) y = last;

		return;
	}

	if (offset == 1) {
		memmove(con_at(0, 0), con_at(0, 1), last * row);
		memset(con_at(0, last), 0, row);

		y --;
		if (y < 0) y = 0;

		return;
	}

}

void con_write(char code) {

	if (code == ANSI_BACKSPACE) {
		if (x > 0) x --;
		return;
	}

	if (code == ANSI_CARRIAGE_RETURN) {
		x = 0;
		return;
	}

	if (code == ANSI_LINE_FEED) {
		y ++;
		x = 0;

		if (y >= height) {
			con_scroll(+1);
		}
		return;
	}

	if (code == ANSI_TAB) {
		x += (x % 5) ? 5 : 0;
		goto adjust;
	}

	uint8_t* glyph = con_at(x, y);

	glyph[0] = code;
	glyph[1] = attribute;

adjust:

	x ++;

	if (x >= width) {
		x = 0;
		y ++;
	}

	if (y >= height) {
		con_scroll(+1);
	}
}
