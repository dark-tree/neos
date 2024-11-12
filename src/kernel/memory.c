#include "memory.h"

void* memcpy(void* dst, const void* src, long bytes) {
	if ((src == dst) || (bytes == 0)) {
		return dst;
	}

	for (long i = 0; i < bytes; i ++) {
		((uint8_t*) dst)[i] = ((uint8_t*) src)[i];
	}

	return dst;
}

void* memmove(void* dst, const void* src, long bytes) {
	if ((src == dst) || (bytes == 0)) {
		return dst;
	}

	if (src > dst) {
		return memcpy(dst, src, bytes);
	}

	for (long i = bytes - 1; i >= 0; i --) {
		((uint8_t*) dst)[i] = ((uint8_t*) src)[i];
	}

	return dst;
}

void* memset(void* dst, uint8_t value, long bytes) {
	for (long i = 0; i < bytes; i ++) {
		((uint8_t*) dst)[i] = value;
	}

	return dst;
}

int strlen(const char* cstr) {
	int length = 0;

	while(cstr[length] != 0x00) {
		length ++;
	}

	return length;
}

int wstrlen(const short* wstr) {
	int length = 0;

	while(wstr[length] != 0x0000) {
		length ++;
	}

	return length;
}
