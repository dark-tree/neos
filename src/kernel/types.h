#pragma GCC system_header

typedef unsigned char bool;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned int size_t;
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

// Compile time checks - Basic Types
_Static_assert ((sizeof (bool) == 1), "bool needs to be 1 byte long!");
_Static_assert ((sizeof (uint8_t) == 1), "uint8_t needs to be 1 byte long!");
_Static_assert ((sizeof (uint16_t) == 2), "uint16_t needs to be 2 bytes long!");
_Static_assert ((sizeof (uint32_t) == 4), "uint32_t needs to be 4 bytes long!");
_Static_assert ((sizeof (uint64_t) == 8), "uint64_t needs to be 8 bytes long!");
_Static_assert ((sizeof (size_t) >= 4), "size_t needs to be at least 4 bytes long!");
_Static_assert ((sizeof (int8_t) == 1), "int8_t needs to be 1 byte long!");
_Static_assert ((sizeof (int16_t) == 2), "int16_t needs to be 2 bytes long!");
_Static_assert ((sizeof (int32_t) == 4), "int32_t needs to be 4 bytes long!");
_Static_assert ((sizeof (int64_t) == 8), "int64_t needs to be 8 bytes long!");

#define true 1
#define false 0
#define NULL 0

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
