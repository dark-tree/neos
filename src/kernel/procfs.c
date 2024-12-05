#include "procfs.h"
#include "kmalloc.h"
#include "errno.h"

/* private */

/* exported */

int procfs_root(vRef* dst) {
	kprintf("procfs: root\n");
}

int procfs_clone(vRef* dst, vRef* src) {
	kprintf("procfs: clone\n");
}

int procfs_open(vRef* vref, const char* part, uint32_t flags) {
	kprintf("procfs: open %s\n", part);
}

int procfs_close(vRef* vref) {
	kprintf("procfs: close\n");
}

int procfs_read(vRef* vref, void* buffer, uint32_t size, uint32_t count) {
	kprintf("procfs: read %d\n", size * count);
}

int procfs_write(vRef* vref, void* buffer, uint32_t size, uint32_t count) {
	kprintf("procfs: write %d\n", size * count);
}

int procfs_seek(vRef* vref, int offset, int whence) {
	kprintf("procfs: seek %d\n", offset);
}

int procfs_list(vRef* vref, vEntry* entries, int max) {
	kprintf("procfs: list\n");
}

int procfs_mkdir(vRef* vref, const char* name) {
	kprintf("procfs: mkdir %s\n", name);
}

int procfs_remove(vRef* vref, bool rmdir) {
	kprintf("procfs: remove\n");
}

int procfs_stat(vRef* vref, vStat* stat) {
	kprintf("procfs: stat\n");
}

int procfs_readlink(vRef* vref, const char* name, const char* buffer, int size) {
	kprintf("procfs: readlink\n");
}

/* public */

void procfs_load(FilesystemDriver* driver) {
	memcpy(driver->identifier, "ProcFS", 6);

	// export driver functions
	driver->root = procfs_root;
	driver->clone = procfs_clone;
	driver->open = procfs_open;
	driver->close = procfs_close;
	driver->read = procfs_read;
	driver->write = procfs_write;
	driver->seek = procfs_seek;
	driver->list = procfs_list;
	driver->mkdir = procfs_mkdir;
	driver->remove = procfs_remove;
	driver->stat = procfs_stat;
	driver->readlink = procfs_readlink;
}
