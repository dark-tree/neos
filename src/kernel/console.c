#include "console.h"
#include "memory.h"
#include "ansi.h"
#include "math.h"

/* private */

#define CONSOLE_MAX_ANSI_SEQUENCE 64
#define CONSOLE_DEFAULT_ATTRIBUTE 0b00000111
#define CONSOLE_TAB_DISTANCE 4

void (*output_lexer) (char);

static char sequence[CONSOLE_MAX_ANSI_SEQUENCE] = {0};
static int length;

// attribute background
#define AB_I 0b10000000
#define AB_R 0b01000000
#define AB_G 0b00100000
#define AB_B 0b00010000
#define AB_C 0b01110000

// attribute foreground
#define AF_I 0b00001000
#define AF_R 0b00000100
#define AF_G 0b00000010
#define AF_B 0b00000001
#define AF_C 0b00000111

static bool invert;
static uint8_t attribute;
static int width, height;
static int x, y, sx, sy;

int next_tab_stop() {
    return (x / CONSOLE_TAB_DISTANCE) * CONSOLE_TAB_DISTANCE + ((x % CONSOLE_TAB_DISTANCE) ? CONSOLE_TAB_DISTANCE : 0);
}

static uint8_t ansi_to_rgb_bits(int index) {
	int color = index & 7;
	return (color & 0b010) | ((color & 0b001) << 2) | ((color & 0b100) >> 2);
}

static uint8_t con_attr() {
	if (!invert) {
		return attribute;
	}

	// inverted mode
	uint8_t top = (0x0F & attribute) << 4;
	return (top | (attribute >> 4)) & 0xFF;
}

static uint8_t* con_at(int x, int y) {
	return (uint8_t*) &((uint16_t*) 0xB8000)[x + y * width];
}

static char con_next_char(int* head, int last) {
	if (*head > last) {
		return 0;
	}

	return sequence[(*head) ++];
}

static int con_next_int(int* head, int last, int fallback) {

	bool first = true;
	int accumulator = 0;

	while (true) {
		char next = con_next_char(head, last);

		if ((next >= '0') && (next <= '9')) {
			accumulator = accumulator * 10 + (next - '0');
		} else {
			(*head) --; // unput non-digit
			return first ? fallback : accumulator;
		}

		first = false;
	}

	// unreachable
	return -1;

}

static void con_srg_apply(int code) {
	if (code == ANSI_SGR_RESET) {
		attribute = CONSOLE_DEFAULT_ATTRIBUTE;
		return;
	}

	if (code == ANSI_SGR_BOLD) {
		attribute |= AF_I;
		return;
	}

	if (code == ANSI_SGR_BOLD_RESET) {
		attribute &= ~AF_I;
		return;
	}

	if (code == ANSI_SGR_INVERT) {
		invert = true;
		return;
	}

	if (code == ANSI_SGR_INVERT_RESET) {
		invert = false;
		return;
	}

	if (code >= ANSI_SGR_FG_COLOR_BEGIN && code <= ANSI_SGR_FG_COLOR_END) {
		int color = ansi_to_rgb_bits(code - ANSI_SGR_FG_COLOR_BEGIN); // last 3 bits
		attribute = (attribute & ~AF_C) | color;
	}

	if (code >= ANSI_SGR_BG_COLOR_BEGIN && code <= ANSI_SGR_BG_COLOR_END) {
		int color = ansi_to_rgb_bits(code - ANSI_SGR_BG_COLOR_BEGIN); // last 3 bits
		attribute = (attribute & ~AB_C) | (color << 4); // move into place
	}

	if (code == ANSI_SGR_FG_COLOR_RESET) {
		attribute = (attribute & ~AF_C) | (CONSOLE_DEFAULT_ATTRIBUTE & AF_C);
	}

	if (code == ANSI_SGR_BG_COLOR_RESET) {
		attribute = (attribute & ~AB_C);
	}

}

static void con_sgr_execute(int* head, int last) {

	bool first = true;

	// read an apply args until we run out
	while (*head < last) {
		int arg = con_next_int(head, last, -1);
		con_next_char(head, last); // skip ';' or 'm'

		if (arg == -1) {
			break;
		}

		con_srg_apply(arg);
		first = false;
	}

	// empty code ESC[m should be treated as ESC[0m
	if (first) {
		con_srg_apply(0);
	}

}

static void con_csi_execute(int* head, int last) {
	char code = sequence[last];

	// move cursor up
	if (code == ANSI_CSI_CUU) {
		int arg = con_next_int(head, last, 1);
		y = max(y - arg, 0);
		return;
	}

	// move cursor down
	if (code == ANSI_CSI_CUD) {
		int arg = con_next_int(head, last, 1);
		y = min(y + arg, height - 1);
		return;
	}

	// move cursor forward
	if (code == ANSI_CSI_CUF) {
		int arg = con_next_int(head, last, 1);
		x = min(x + arg, width - 1);
		return;
	}

	// move cursor backward
	if (code == ANSI_CSI_CUB) {
		int arg = con_next_int(head, last, 1);
		x = max(x - arg, 0);
		return;
	}

	// cursor next line
	if (code == ANSI_CSI_CNL) {
		int arg = con_next_int(head, last, 1);
		x = 0;
		y = min(y + arg, height - 1);
		return;
	}

	// cursor previous line
	if (code == ANSI_CSI_CNL) {
		int arg = con_next_int(head, last, 1);
		x = 0;
		y = min(y + arg, height - 1);
		return;
	}

	// cursor position
	if (code == ANSI_CSI_CUP || code == ANSI_CSI_HVP) {
		int n = con_next_int(head, last, 1) - 1;
		con_next_char(head, last); // ';'
		int m = con_next_int(head, last, 1) - 1;

		x = clamp(n, 0, width - 1);
		y = clamp(n, 0, height - 1);
		return;
	}

	// erase in display
	if (code == ANSI_CSI_ED) {
		int n = con_next_int(head, last, 0);
		// TODO
		return;
	}

	// erase in line
	if (code == ANSI_CSI_EL) {
		int n = con_next_int(head, last, 0);
		// TODO
		return;
	}

	// shift screen up
	if (code == ANSI_CSI_SU) {
		int n = con_next_int(head, last, 1);
		// TODO
		con_scroll(-n);
		return;
	}

	// shift screen down
	if (code == ANSI_CSI_SD) {
		int n = con_next_int(head, last, 1);
		// TODO
		con_scroll(+n);
		return;
	}

	// save cursor position
	if (code == ANSI_CSI_SCP) {
		sx = x;
		sy = y;
		return;
	}

	// restore cursor position
	if (code == ANSI_CSI_SCP) {
		x = sx;
		y = sy;
		return;
	}

	// color control
	if (code == ANSI_CSI_SGR) {
		con_sgr_execute(head, last);
	}
}

static void con_execute() {

	int last = length - 1;
	int head = 1;

	// for now we mostly only handle those and ignore the rest
	if (sequence[0] == ANSI_CSI) {
		con_csi_execute(&head, last);
	}

}

/* lexers */

static void con_lexer_initial(char code);
static void con_lexer_escape(char code);
static void con_lexer_csi(char code);
static void con_lexer_until_st(char code);

static void con_lexer_initial(char code) {

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

	if (code == ANSI_ESCAPE) {
		output_lexer = con_lexer_escape;
		return;
	}

	if (code == ANSI_ESC_CSI) {
		length = 0;
		memset(sequence, 0, CONSOLE_MAX_ANSI_SEQUENCE);
		sequence[length ++] = ANSI_CSI;

		output_lexer = con_lexer_csi;
		return;
	}

	if (code == ANSI_ESC_DCS || code == ANSI_ESC_OCS) {
		length = 0;
		memset(sequence, 0, CONSOLE_MAX_ANSI_SEQUENCE);
		sequence[length ++] = ANSI_CSI;

		output_lexer = con_lexer_until_st;
		return;
	}

	if (code == ANSI_TAB) {
		x = next_tab_stop();
		goto adjust;
	}

	uint8_t* glyph = con_at(x, y);

	glyph[0] = code;
	glyph[1] = con_attr();

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

static void con_lexer_escape(char code) {

	length = 0;
	memset(sequence, 0, CONSOLE_MAX_ANSI_SEQUENCE);
	sequence[length ++] = code;

	if (code == ANSI_CSI) {
		output_lexer = con_lexer_csi;
		return;
	}

	if (code == ANSI_OSC || code == ANSI_SOS || code == ANSI_PM || code == ANSI_APC || code == ANSI_DCS) {
		output_lexer = con_lexer_until_st;
		return;
	}

	// unknown code, go back to printing
	output_lexer = con_lexer_initial;
	return;

}

static void con_lexer_csi(char code) {
	sequence[length ++] = code;

	if (ANSI_CSI_FINAL(code)) {
		con_execute();
		output_lexer = con_lexer_initial;
	}

	// error! we run out of space for ANSI control sequence
	// we possibly now could just ignore everything until ANSI_CSI_FINAL
	if (length >= CONSOLE_MAX_ANSI_SEQUENCE) {
		output_lexer = con_lexer_initial;
	}
}

static void con_lexer_until_st(char code) {
	sequence[length ++] = code;

	if (code == ANSI_ST && length > 1 && sequence[length - 2] == ANSI_ESCAPE) {
		con_execute();
		output_lexer = con_lexer_initial;
	}

	// error! we run out of space for ANSI control sequence
	// we possibly now could just ignore everything until ESC+ST
	if (length >= CONSOLE_MAX_ANSI_SEQUENCE) {
		output_lexer = con_lexer_initial;
	}
}

/* public */

void con_init(int max_width, int max_height) {
	width = max_width;
	height = max_height;
	x = 0;
	y = 0;
	attribute = CONSOLE_DEFAULT_ATTRIBUTE;
	output_lexer = con_lexer_initial;
	length = 0;
	sx = 0;
	sy = 0;
	invert = false;
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
	output_lexer(code);
}
