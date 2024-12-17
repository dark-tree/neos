
#include "rivendell.h"
#include "print.h"
#include "memory.h"
#include "kmalloc.h"
#include "math.h"

/* private */

typedef struct {
	uint32_t file_offset;
	uint32_t mem_offset;
	uint32_t file_size;
	uint32_t mem_size;
} ImageSegment;

typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef int32_t Elf32_Sword;
typedef uint32_t Elf32_Word;

#define ELFCLASSNONE 0
#define ELFCLASS32 1
#define ELFCLASS64 2

#define ELFDATANONE 0
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2

// Machine Types
#define EM_NONE 0
#define EM_M32 1
#define EM_SPARC 2
#define EM_386 3
#define EM_68K 4
#define EM_88K 5
#define EM_860 7 /* 6 omited as per the spec */
#define EM_MIPS 8

// Segment Types
#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_SHLIB 5
#define PT_PHDR 6

typedef struct {
	uint8_t magic[4];
	uint8_t class;
	uint8_t encoding;
	uint8_t version;
	uint8_t padding[9];
} ABI_PACKED Elf32_Ident;

typedef struct {
	Elf32_Ident ident;
	Elf32_Half  type;
	Elf32_Half  machine;
	Elf32_Word  version;
	Elf32_Addr  entry;
	Elf32_Off   phoff;
	Elf32_Off   shoff;
	Elf32_Word  flags;
	Elf32_Half  ehsize;
	Elf32_Half  phentsize;
	Elf32_Half  phnum;
	Elf32_Half  shentsize;
	Elf32_Half  shnum;
	Elf32_Half  shstrndx;
} ABI_PACKED Elf32_Ehdr;

typedef struct {
	Elf32_Word  type;
	Elf32_Off   offset;
	Elf32_Addr  vaddr;
	Elf32_Addr  paddr;
	Elf32_Word  filesz;
	Elf32_Word  memsz;
	Elf32_Word  flags;
	Elf32_Word  align;
} ABI_PACKED Elf32_Phdr;

// Compile time checks - Basic Types
_Static_assert ((sizeof (Elf32_Addr) == 4), "Elf32_Addr needs to be 4 bytes long!");
_Static_assert ((sizeof (Elf32_Half) == 2), "Elf32_Half needs to be 2 bytes long!");
_Static_assert ((sizeof (Elf32_Off) == 4), "Elf32_Off needs to be 4 bytes long!");
_Static_assert ((sizeof (Elf32_Sword) == 4), "Elf32_Sword needs to be 4 bytes long!");
_Static_assert ((sizeof (Elf32_Word) == 4), "Elf32_Word needs to be 4 bytes long!");

// Compile time checks - Structs
_Static_assert ((sizeof (Elf32_Ident) == 16), "Elf32_Ident needs to be 16 bytes long!");
_Static_assert ((sizeof (Elf32_Ehdr) == 52), "Elf32_Ehdr needs to be 72 bytes long!");

static const char* elf_phtypestr(int type) {
	if (type == PT_NULL)    return "NULL";
	if (type == PT_LOAD)    return "LOAD";
	if (type == PT_DYNAMIC) return "DYNM";
	if (type == PT_INTERP)  return "INTR";
	if (type == PT_NOTE)    return "NOTE";
	if (type == PT_SHLIB)   return "SLIB";
	if (type == PT_SHLIB)   return "PHDR";

	return "????";
}

static int elf_checkident(Elf32_Ident* ident) {

	if (ident->magic[0] != 0x7f) return 1;
	if (ident->magic[1] != 'E') return 1;
	if (ident->magic[2] != 'L') return 1;
	if (ident->magic[3] != 'F') return 1;
	if (ident->class != ELFCLASS32) return 1;
	if (ident->encoding != ELFDATA2LSB) return 1;
	if (ident->version != 1) return 1;

	// all OK
	return 0;
}

static int elf_segcpy(vRef* vref, void* image, uint32_t mount, ImageSegment* segment, bool verbose) {

	// seek to the segment contents
	vfs_seek(vref, segment->file_offset, SEEK_SET);

	uint32_t address = segment->mem_offset - mount;
	uint32_t content = min(segment->file_size, segment->mem_size);
	uint32_t padding = max(0, (long) segment->mem_size - (long) segment->file_size);

	if (verbose) {
		kprintf("[+0x%0.8x] Copying %ud file bytes, and %ud null bytes\n", address, content, padding);
	}

	if (vfs_read(vref, image + address, content) == 0) {
		return ELF_READ_ERROR;
	}

	memset(image + address, 0, segment->mem_size);
	return ELF_SUCCESS;
}

static int elf_segments(vRef* vref, const Elf32_Ehdr* header, ProgramImage* program, bool verbose) {

	if (verbose) {
		kprintf("\nSegments: \n");
		kprintf("\e[1m   type align    offset   vaddr    filesz   memsz\e[m\n");
	}

	// copy of loadable segments for future use
	// used to not read the file multiple times
	int phlcnt = 0;
	ImageSegment* phlptr = kmalloc(sizeof(ImageSegment) * header->phnum);

	// memory bounds, needed for the final image allocation
	uint32_t low = -1;
	uint32_t high = 0;

	// read segments from file
	for (int i = 0; i < header->phnum; i ++) {

		Elf32_Phdr entry;

		// seek to the Program Header Table entry
		vfs_seek(vref, header->phoff + i * header->phentsize, SEEK_SET);

		if (vfs_read(vref, &entry, sizeof(Elf32_Phdr)) == 0) {
			return ELF_READ_ERROR;
		}

		// found loadable segment, copy for later use
		if (entry.type == PT_LOAD) {
			phlptr[phlcnt].file_offset = entry.offset;
			phlptr[phlcnt].file_size = entry.filesz;
			phlptr[phlcnt].mem_offset = entry.vaddr;
			phlptr[phlcnt].mem_size = entry.memsz;

			uint32_t begin = entry.vaddr;
			uint32_t end = begin + entry.memsz;

			if (begin < low) low = begin;
			if (end > high) high = end;

			phlcnt ++;
		}

		if (verbose) {
			kprintf(" * %s %0.8x %0.8x %0.8x %0.8x %0.8x\n", elf_phtypestr(entry.type), entry.align, entry.offset, entry.vaddr, entry.filesz, entry.memsz);
		}
	}

	uint32_t bytes = high - low;

	if (verbose) {
		kprintf("\nProgram: \n");
		kprintf(" * phlcnt : %ud\n", phlcnt);
		kprintf(" * memory : %#0.8x:%#0.8x (%d bytes)\n", low, high, bytes);
		kprintf("\n");
	}

	void* image = kmalloc(bytes + program->prefix + program->sufix);

	for (int i = 0; i < phlcnt; i ++) {
		int err;

		if ((err = elf_segcpy(vref, image + program->prefix, low, phlptr + i, verbose)) != ELF_SUCCESS) {
			kfree(phlptr);
			return err;
		}
	}

	program->image = image;
	program->mount = low;

	kfree(phlptr);
	return ELF_SUCCESS;

}

/* public */

const char* elf_err(int err) {
	if (err == ELF_READ_ERROR) return "read error";
	if (err == ELF_IDENT_ERROR) return "invalid file identification";
	if (err == ELF_WRONG_MACHINE_ERROR) return "not a Intel 383 file";
	if (err == ELF_HEADER_ERROR) return "invalid data in header";

	return "";
}


int elf_load(vRef* vref, ProgramImage* image, bool verbose) {

	Elf32_Ehdr header;

	if (vfs_read(vref, &header, sizeof(Elf32_Ehdr)) == 0) {
		return ELF_READ_ERROR;
	}

	// check if the first 16 bytes are what we expect
	if (elf_checkident(&header.ident) == 1) {
		return ELF_IDENT_ERROR;
	}

	// check if the file was compiled for Intel 386
	if ((header.machine != EM_386) && (header.machine != EM_NONE)) {
		return ELF_WRONG_MACHINE_ERROR;
	}

	// check the version again
	if (header.version != 1) {
		return ELF_HEADER_ERROR;
	}

	// check header size
	if (header.ehsize < sizeof(Elf32_Ehdr)) {
		return ELF_HEADER_ERROR;
	}

	// check program header entry size
	if (header.phentsize < sizeof(Elf32_Phdr)) {
		return ELF_HEADER_ERROR;
	}

//	// check section header entry size
//	if (header.shentsize < sizeof(Elf32_Ehdr)) {
//		return ELF_HEADER_ERROR;
//	}

	if (verbose) {
		kprintf("Header: \n");
		kprintf(" * entry    : %#0.8x\n", (uint32_t) header.entry);
		kprintf(" * phoff    : %#0.8x (count: %0.2d, size: %0.2d)\n", (uint32_t) header.phoff, (uint32_t) header.phnum, (uint32_t) header.phentsize);
		kprintf(" * shoff    : %#0.8x (count: %0.2d, size: %0.2d)\n", (uint32_t) header.shoff, (uint32_t) header.shnum, (uint32_t) header.shentsize);
		kprintf(" * shstrndx : %ud\n", (uint32_t) header.shstrndx);
	}

	int err;

	if ((err = elf_segments(vref, &header, image, verbose)) != ELF_SUCCESS) {
		return err;
	}

	image->entry = header.entry;
	return ELF_SUCCESS;

}
