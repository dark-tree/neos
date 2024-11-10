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

void kprint_string(PrintState* state, const char* string) {
	int length = strlen(string);

	int padding = 0;

	if (state->width != -1) {
		padding = max(0, state->width - length);
	}

	for (int i = 0; i < padding; i ++) {
		con_write(state->padding);
	}

	for (int i = 0; i < length; i ++) {
		char chr = string[i];

		if (chr == '\0') {
			break;
		}

		con_write(chr);
	}
}

void kprint_integer(PrintState* state, long integer, int base) {
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

	while (uint && i) {
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
