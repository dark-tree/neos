
#include "types.h"
#include "console.h"
#include "floppy.h"
#include "print.h"
#include "interrupt.h"
#include "util.h"
#include "cursor.h"
#include "mem.h"
#include "scheduler.h"
#include "vfs.h"
#include "memory.h"
#include "procfs.h"
#include "fatfs.h"
#include "routine.h"
#include "kmalloc.h"
#include "fat.h"
#include "rivendell.h"

void start() __attribute__((section(".text.start")));

extern char asm_test();
extern void pic_disable();

extern void* getprocess1();
extern void* getprocess2();

#define X "\xDB"
#define S " "

void start() {

	con_init(80, 25);
	cur_enable();

	// Init memory system and make room for the kernel
	mem_init(0xFFFFF + 1 /* TODO: remove when fix is merged */);
	pic_disable();
	int_init();

	isr_register(0x26, NULL);

//	kprintf("\e[2J%% Hello \e[1;33m%s\e[m wo%cld, party like it's \e[1m%#0.8x\e[m again!\n", "sweet", 'r', -1920);

//	kprintf("\e[4B");
//	kprintf("\e[29C" X S S S X S X X X X S S X X X S S X X X X"\n");
//	kprintf("\e[29C" X X S S X S S S S S S X S S S X S S S S S"\n");
//	kprintf("\e[29C" X S X S X S X X S S S X S X S X S X X X X"\n");
//	kprintf("\e[29C" X S S X X S S S S S S X S S S X S S S S X"\n");
//	kprintf("\e[29C" X S S S X S X X X X S S X X X S S X X X X"\n");
//	kprintf("\e[29C" " Linux Compatible OS\n");

	vfs_init();

	// for now mount /proc at /
	FilesystemDriver procfs;
	//procfs_load(&procfs);
	fatfs_load(&procfs);
	vfs_mount("/", &procfs);

	vfs_print(NULL, 0);

	kprintf("System ready!\n");

	vRef root = vfs_root();
	//vfs_open(root, "/testing/omg/tmp/test.txt");
	vRef ref;

	int res = vfs_open(&ref, &root, "./abcd/../tmp/haha.txt", 0);
	kprintf("Return: %d\n", res);

	//vfs_open(&ref, &root, "./abc/foo/test.txt", OPEN_CREAT);
	//res = vfs_write(&ref, "Hello, World!\n", 14);
	//kprintf("Return: %d\n", res);
	//res = vfs_write(&ref, "Hello, World!\n", 14);
	//kprintf("Return: %d\n", res);

	vfs_open(&ref, &root, "./executable/elf", 0);
	int size = vfs_seek(&ref, 0, SEEK_END);
	vfs_seek(&ref, 0, SEEK_SET);
	//char* buffer = kmalloc(size);
	//vfs_read(&ref, buffer, size);

	// ELF file is loaded!
	//fat_print_buffer(buffer, 128);

	ProgramImage image;
	elf_load(&ref, &image);


	//kfree(buffer);

	halt();

	scheduler_init();
	scheduler_create_process(-1, getprocess1());
	scheduler_create_process(-1, getprocess2());

	// never return to the bootloader
	halt();
}
