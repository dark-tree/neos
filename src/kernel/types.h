#pragma GCC system_header

typedef unsigned char bool;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned int size_t;

#define true 1
#define false 0

// Helpers to control the binary interface
#define ABI_PACKED __attribute__((packed, aligned(1)))
#define ABI_CDECL __attribute__((__cdecl__))

// varargs
#ifndef va_start
typedef __builtin_va_list va_list;

#	define va_start __builtin_va_start
#	define va_end __builtin_va_end
#	define va_copy __builtin_va_copy
#	define va_arg __builtin_va_arg
#endif
