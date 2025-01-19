
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

#include "gdt.h"

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
	mem_init(0xFFFFF);

    ginit();

	pic_disable();
	int_init();

//  kprintf("\e[2J%% Hello \e[1;33m%s\e[m wo%cld, party like it's \e[1m%#0.8x\e[m again!\n", "sweet", 'r', -1920);
//	kprintf("\e[4B");
//	kprintf("\e[29C" X S S S X S X X X X S S X X X S S X X X X"\n");
//	kprintf("\e[29C" X X S S X S S S S S S X S S S X S S S S S"\n");
//	kprintf("\e[29C" X S X S X S X X S S S X S X S X S X X X X"\n");
//	kprintf("\e[29C" X S S X X S S S S S S X S S S X S S S S X"\n");
//	kprintf("\e[29C" X S S S X S X X X X S S X X X S S X X X X"\n");
//	kprintf("\e[29C" " Linux Compatible OS\n");

	vfs_init();

	FilesystemDriver procfs;
    procfs_load(&procfs);
    vfs_mount("/proc/", &procfs);

    FilesystemDriver fatfs;
    fatfs_load(&fatfs);
    vfs_mount("/", &fatfs);

	vfs_print(NULL, 0);

	kprintf("System ready!\n");

    vRef root = vfs_root();
    vRef elf;
    vfs_open(&elf, &root, "/executable/tiny", 0);

    scheduler_init();

    scheduler_create_process(-1, &elf);



	// never return to the bootloader
	halt();
}
