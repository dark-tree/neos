#pragma once

#include "types.h"
#include "vfs.h"

#define ELF_SUCCESS 0
#define ELF_READ_ERROR -1
#define ELF_IDENT_ERROR -2
#define ELF_WRONG_MACHINE_ERROR -3
#define ELF_HEADER_ERROR -4

typedef struct {
	uint32_t prefix; // number of bytes allocated before the image
	uint32_t sufix;  // number of bytes allocated after the image
	uint32_t entry;  // virtual address the execution should begin at
	uint32_t mount;  // the virtual address the image must be mounted at

	void* image;
} ProgramImage;

const char* elf_err(int err);
int elf_load(vRef* file, ProgramImage* image, bool verbose);
