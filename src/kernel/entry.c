
#include "types.h"
#include "console.h"
#include "print.h"
#include "interrupt.h"
#include "util.h"
#include "cursor.h"
#include "mem.h"
#include "vfs.h"
#include "memory.h"
#include "procfs.h"

extern char asm_test();
extern void pic_disable();

#define X "\xDB"
#define S " "

void start() {
	con_init(80, 25);
	cur_enable();

	// Init memory system and make room for the kernel
	mem_init(0xFFFFF + 1);

//	kprintf("\e[2J%% Hello \e[1;33m%s\e[m wo%cld, party like it's \e[1m%#0.8x\e[m again!\n", "sweet", 'r', -1920);

//	kprintf("\e[4B");
//	kprintf("\e[29C" X S S S X S X X X X S S X X X S S X X X X"\n");
//	kprintf("\e[29C" X X S S X S S S S S S X S S S X S S S S S"\n");
//	kprintf("\e[29C" X S X S X S X X S S S X S X S X S X X X X"\n");
//	kprintf("\e[29C" X S S X X S S S S S S X S S S X S S S S X"\n");
//	kprintf("\e[29C" X S S S X S X X X X S S X X X S S X X X X"\n");
//	kprintf("\e[29C" " Linux Compatible OS\n");

	pic_disable();
	int_init();

	vfs_init();

	// for now mount /proc at /
	FilesystemDriver procfs;
	procfs_load(&procfs);
	vfs_mount("/", &procfs);

	vfs_print(NULL, 0);

	kprintf("System ready!\n");

//	vRef root;
//	root.offset = 0;
//	root.node = &vfs_root;
//	root.driver = NULL;
//	root.state = NULL;

	vRef root = vfs_root();
	//vfs_open(root, "/testing/omg/tmp/test.txt");
	vRef ref;

	int res = vfs_open(&ref, &root, "./abcd/../tmp/haha.txt", 0);

	kprintf("Return: %d\n", res);

	//asm_test();

	// never return to the bootloader
	halt();
}
