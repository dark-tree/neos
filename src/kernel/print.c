#include "print.h"
#include "console.h"
#include "types.h"

/* private */

void kprint_string(const char* string) {
	for (int i = 0; true; i ++) {
		char chr = string[i];

		if (chr == '\0') {
			break;
		}

		con_write(chr);
	}
}

void kprint_integer(long integer, int base) {
	const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	char buffer[64] = {0};
	int i = 30;

	// early return
	if (integer == 0) {
		con_write('0');
		return;
	}

	if (integer < 0) {
		con_write('-');
		integer = -integer;
	}

	while (integer && i) {
		long rem = integer % base;
		buffer[i--] = digits[rem];
		integer /= base;
	}

	kprint_string(&buffer[i + 1]);
}

/* public */

void kprintf(const char* pattern, ...) {
	va_list args;
	va_start(args, pattern);

	bool escape = false;

	for (int i = 0; true; i ++) {
		char chr = pattern[i];

		if (chr == '\0') {
			break;
		}

		if (escape) {
			escape = false;

			if (chr == '%') {
				con_write(chr);
				continue;
			}

			if (chr == 's') {
				kprint_string(va_arg(args, const char*));
				continue;
			}

			if (chr == 'b') {
				kprint_integer(va_arg(args, int), 2);
				continue;
			}

			if (chr == 'o') {
				kprint_integer(va_arg(args, int), 8);
				continue;
			}

			if (chr == 'd' || chr == 'i') {
				kprint_integer(va_arg(args, int), 10);
				continue;
			}

			if (chr == 'x') {
				kprint_integer(va_arg(args, int), 16);
				continue;
			}

			if (chr == 'c') {
				con_write(va_arg(args, int) & 0xFF);
				continue;
			}

			continue;
		}

		if (chr == '%') {
			escape = true;
			continue;
		}

		con_write(chr);
	}

	va_end(args);
}
