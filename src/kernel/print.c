#include "print.h"
#include "console.h"
#include "types.h"
#include "config.h"
#include "math.h"
#include "memory.h"
#include "util.h"

/* private */

typedef enum {
	PRINT_DEFAULT,
	PRINT_ESCAPE,
	PRINT_PADDING
} PrintMode;

typedef struct {
	PrintMode mode; // used during parsing only
	bool prefix;    // prefix numbers with the base specifier
	bool sign;      // force sign
	bool space;     // if no sign is going to be written print a single space
	bool usign;     // interpret the value as an unsigned value
	char padding;   // the character used to right-justify values
	int width;      // the minimum length of a value expansion
} PrintState;

static char u16_to_cp437(short chr) {

	if (chr >= ' ' && chr <= '~') {
		return chr;
	}

	if (chr == 0x2302) return 0x7F; // House
	if (chr == 0x00C7) return 0x80; // Latin Capital Letter C with Cedilla
	if (chr == 0x00FC) return 0x81; // Latin Small Letter U with Diaeresis
	if (chr == 0x00E9) return 0x82; // Latin Small Letter E with Acute
	if (chr == 0x00E2) return 0x83; // Latin Small Letter A with Circumflex
	if (chr == 0x00E4) return 0x84; // Latin Small Letter A with Diaeresis
	if (chr == 0x00E0) return 0x85; // Latin Small Letter A with Grave
	if (chr == 0x00E5) return 0x86; // Latin Small Letter A with Ring Above
	if (chr == 0x00E7) return 0x87; // Latin Small Letter C with Cedilla
	if (chr == 0x00EA) return 0x88; // Latin Small Letter E with Circumflex
	if (chr == 0x00EB) return 0x89; // Latin Small Letter E with Diaeresis
	if (chr == 0x00E8) return 0x8A; // Latin Small Letter E with Grave
	if (chr == 0x00EF) return 0x8B; // Latin Small Letter I with Diaeresis
	if (chr == 0x00EE) return 0x8C; // Latin Small Letter I with Circumflex
	if (chr == 0x00EC) return 0x8D; // Latin Small Letter I with Grave
	if (chr == 0x00C4) return 0x8E; // Latin Capital Letter A with Diaeresis
	if (chr == 0x00C5) return 0x8F; // Latin Capital Letter A with Ring Above

	if (chr == 0x00C9) return 0x90; // Latin Capital Letter E with Acute
	if (chr == 0x00E6) return 0x91; // Latin Small Letter Ae
	if (chr == 0x00C6) return 0x92; // Latin Capital Letter Ae
	if (chr == 0x00F4) return 0x93; // Latin Small Letter O with Circumflex
	if (chr == 0x00F6) return 0x94; // Latin Small Letter O with Diaeresis
	if (chr == 0x00F2) return 0x95; // Latin Small Letter O with Grave
	if (chr == 0x00FB) return 0x96; // Latin Small Letter U with Circumflex
	if (chr == 0x00F9) return 0x97; // Latin Small Letter U with Grave
	if (chr == 0x00FF) return 0x98; // Latin Small Letter Y with Diaeresis
	if (chr == 0x00D6) return 0x99; // Latin Capital Letter O with Diaeresis
	if (chr == 0x00DC) return 0x9A; // Latin Capital Letter U with Diaeresis
	if (chr == 0x00A2) return 0x9B; // Cent Sign
	if (chr == 0x00A3) return 0x9C; // Pound Sign
	if (chr == 0x00A5) return 0x9D; // Yen Sign
	if (chr == 0x20A7) return 0x9E; // Peseta Sign
	if (chr == 0x0192) return 0x9F; // Latin Small Letter F with Hook

	if (chr == 0x00E1) return 0xA0; // Latin Small Letter A with Acute
	if (chr == 0x00ED) return 0xA1; // Latin Small Letter I with Acute
	if (chr == 0x00F3) return 0xA2; // Latin Small Letter O with Acute
	if (chr == 0x00FA) return 0xA3; // Latin Small Letter U with Acute
	if (chr == 0x00F1) return 0xA4; // Latin Small Letter N with Tilde
	if (chr == 0x00D1) return 0xA5; // Latin Capital Letter N with Tilde
	if (chr == 0x00AA) return 0xA6; // Feminine Ordinal Indicator
	if (chr == 0x00BA) return 0xA7; // Masculine Ordinal Indicator
	if (chr == 0x00BF) return 0xA8; // Inverted Question Mark
	if (chr == 0x2310) return 0xA9; // Reversed Not Sign
	if (chr == 0x00AC) return 0xAA; // Not Sign
	if (chr == 0x00BD) return 0xAB; // Vulgar Fraction One Half
	if (chr == 0x00BC) return 0xAC; // Vulgar Fraction One Quarter
	if (chr == 0x00A1) return 0xAD; // Inverted Exclamation Mark
	if (chr == 0x00AB) return 0xAE; // Left-Pointing Double Angle Quotation Mark
	if (chr == 0x00BB) return 0xAF; // Right-Pointing Double Angle Quotation Mark

	if (chr == 0x03B1) return 0xE0; // Greek Small Letter Alpha
	if (chr == 0x00DF) return 0xE1; // Latin Small Letter Sharp S
	if (chr == 0x0393) return 0xE2; // Greek Capital Letter Gamma
	if (chr == 0x03C0) return 0xE3; // Greek Small Letter Pi
	if (chr == 0x03A3) return 0xE4; // Greek Capital Letter Sigma
	if (chr == 0x03C3) return 0xE5; // Greek Small Letter Sigma
	if (chr == 0x00B5) return 0xE6; // Micro Sign
	if (chr == 0x03C4) return 0xE7; // Greek Small Letter Tau
	if (chr == 0x03A6) return 0xE8; // Greek Capital Letter Phi
	if (chr == 0x0398) return 0xE9; // Greek Capital Letter Theta
	if (chr == 0x03A9) return 0xEA; // Greek Capital Letter Omega
	if (chr == 0x03B4) return 0xEB; // Greek Small Letter Delta
	if (chr == 0x221E) return 0xEC; // Infinity
	if (chr == 0x03C6) return 0xED; // Greek Small Letter Phi
	if (chr == 0x03B5) return 0xEE; // Greek Small Letter Epsilon
	if (chr == 0x2229) return 0xEF; // Intersection

	if (chr == 0x2261) return 0xF0; // Identical To
	if (chr == 0x00B1) return 0xF1; // Plus-Minus Sign
	if (chr == 0x2265) return 0xF2; // Greater-Than or Equal To
	if (chr == 0x2264) return 0xF3; // Less-Than or Equal To
	if (chr == 0x2320) return 0xF4; // Top Half Integral
	if (chr == 0x2321) return 0xF5; // Bottom Half Integral
	if (chr == 0x00F7) return 0xF6; // Division Sign
	if (chr == 0x2248) return 0xF7; // Almost Equal To
	if (chr == 0x00B0) return 0xF8; // Degree Sign
	if (chr == 0x2219) return 0xF9; // Bullet Operator
	if (chr == 0x00B7) return 0xFA; // Middle Dot
	if (chr == 0x221A) return 0xFB; // Square Root
	if (chr == 0x207F) return 0xFC; // Superscript Latin Small Letter N
	if (chr == 0x00B2) return 0xFD; // Superscript Two
	if (chr == 0x25A0) return 0xFE; // Black Square
	if (chr == 0x00A0) return 0xFF; // No-Break Space

	if (chr == 0x212B) return 0x8F; // Angstrom Sign

	return '?';
}

static void kprint_wstring(PrintState* state, const uint16_t* string) {
	int length = wstrlen(string);
	int padding = 0;

	if (state->width != -1) {
		padding = max(0, state->width - length);
	}

	for (int i = 0; i < padding; i ++) {
		con_write(state->padding);
	}

	// enter extended mode, ignore the shortcut ANSI sequences
	kprintf("\e<");

	for (int i = 0; i < length; i ++) {
		con_write(u16_to_cp437(string[i]));
	}

	// exit extended mode
	kprintf("\e>");
}

static void kprint_string(PrintState* state, const char* string) {
	int length = strlen(string);
	int padding = 0;

	if (state->width != -1) {
		padding = max(0, state->width - length);
	}

	for (int i = 0; i < padding; i ++) {
		con_write(state->padding);
	}

	for (int i = 0; i < length; i ++) {
		con_write(string[i]);
	}
}

static void kprint_integer(PrintState* state, long integer, int base) {
	const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	char buffer[KPRINTF_MAX_DIGITS] = {0};
	int i = KPRINTF_MAX_DIGITS - 2;
	unsigned long uint = (unsigned long) integer;

	if (state->width == 0 && integer == 0) {
		return;
	}

	if (integer < 0 && !state->usign) {
		con_write('-');
		uint = -integer;
	} else if (state->sign) {
		con_write('+');
	} else if (state->space) {
		con_write(' ');
	}

	// print base prefix
	if (state->prefix) {
		state->padding = '0';

		if (base == 16) {
			con_write('0');
			con_write('x');
		}

		if (base == 8) {
			con_write('0');
			con_write('o');
		}

		if (base == 2) {
			con_write('0');
			con_write('b');
		}
	}

	if (uint == 0) {
		buffer[i--] = '0';
	} else while (uint && i) {
		long rem = uint % base;
		buffer[i--] = digits[rem];
		uint /= base;
	}

	kprint_string(state, &buffer[i + 1]);
}

/* public */

void kprintf(const char* pattern, ...) {
	va_list args;
	va_start(args, pattern);

	PrintState state;
	state.mode = PRINT_DEFAULT;

	for (int i = 0; true; i ++) {
		char chr = pattern[i];

		if (chr == '\0') {
			break;
		}

		// parse padding length
		// this is triggered by the `%.` sequence
		if (state.mode == PRINT_PADDING) {
			if (chr >= '0' && chr <= '9') {
				int digit = chr - '0';

				state.width *= 10;
				state.width += digit;
				continue;
			}

			state.mode = PRINT_ESCAPE;
		}

		// parse specifier and flags
		if (state.mode == PRINT_ESCAPE) {
			state.mode = PRINT_DEFAULT;

			if (chr == '0') {
				state.mode = PRINT_ESCAPE;
				state.padding = '0';
				continue;
			}

			if (chr == '#') {
				state.mode = PRINT_ESCAPE;
				state.prefix = true;
				continue;
			}

			if (chr == '+') {
				state.mode = PRINT_ESCAPE;
				state.sign = true;
				continue;
			}

			if (chr == ' ') {
				state.mode = PRINT_ESCAPE;
				state.space = true;
				continue;
			}

			if (chr == 'u') {
				state.mode = PRINT_ESCAPE;
				state.usign = true;
				continue;
			}

			if (chr == '.') {
				state.mode = PRINT_PADDING;
				state.width = 0;
				continue;
			}

			if (chr == '%') {
				con_write(chr);
				continue;
			}

			if (chr == 's') {
				kprint_string(&state, va_arg(args, const char*));
				continue;
			}

			if (chr == 'S') {
				kprint_wstring(&state, va_arg(args, const uint16_t*));
				continue;
			}

			if (chr == 'b') {
				kprint_integer(&state, va_arg(args, int), 2);
				continue;
			}

			if (chr == 'o') {
				kprint_integer(&state, va_arg(args, int), 8);
				continue;
			}

			if (chr == 'd' || chr == 'i') {
				kprint_integer(&state, va_arg(args, int), 10);
				continue;
			}

			if (chr == 'x') {
				kprint_integer(&state, va_arg(args, int), 16);
				continue;
			}

			if (chr == 'c') {
				con_write(va_arg(args, int) & 0xFF);
				continue;
			}

			continue;
		}

		if (chr == '%') {
			state.mode = PRINT_ESCAPE;
			state.padding = ' ';
			state.width = -1;
			state.sign = false;
			state.prefix = false;
			state.space = false;
			state.usign = false;
			continue;
		}

		con_write(chr);
	}

	va_end(args);
}
