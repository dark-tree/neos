#include "procfs.h"
#include "kmalloc.h"
#include "errno.h"
#include "print.h"
#include "memory.h"

/* private */

/* exported */

int procfs_root(vRef* dst) {
	kprintf("procfs: root\n");
	return 0; // TODO
}

int procfs_clone(vRef* dst, vRef* src) {
	kprintf("procfs: clone\n");
	return 0; // TODO
}

int procfs_open(vRef* vref, const char* basename, uint32_t flags) {
	kprintf("procfs: open %s\n", basename);
	return 0; // TODO
}

int procfs_close(vRef* vref) {
	kprintf("procfs: close\n");
	return 0;
}

int procfs_read(vRef* vref, void* buffer, uint32_t size) {
	kprintf("procfs: read %d\n", size);
	return LINUX_EIO; // TODO
}

int procfs_write(vRef* vref, void* buffer, uint32_t size) {
	kprintf("procfs: write %d\n", size);
	return LINUX_EROFS; // never allow writing to ProcFS
}

int procfs_seek(vRef* vref, int offset, int whence) {
	kprintf("procfs: seek %d\n", offset);
	return LINUX_EIO; // TODO
}

int procfs_list(vRef* vref, vEntry* entries, int max) {
	kprintf("procfs: list\n"); // TODO
	return LINUX_EIO;
}

int procfs_mkdir(vRef* vref, const char* name) {
	kprintf("procfs: mkdir %s\n", name);
	return LINUX_EROFS; // never allow writing to ProcFS
}

int procfs_remove(vRef* vref, bool rmdir) {
	kprintf("procfs: remove\n");
	return LINUX_EROFS; // never allow writing to ProcFS
}

int procfs_stat(vRef* vref, vStat* stat) {
	kprintf("procfs: stat\n");
	return LINUX_EIO; // TODO
}

int procfs_readlink(vRef* vref, const char* name, char* buffer, int size) {
	kprintf("procfs: readlink\n");
	return LINUX_EIO; // TODO
}

/* public */

void procfs_load(FilesystemDriver* driver) {
	memcpy(driver->identifier, "ProcFS", 7);

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
