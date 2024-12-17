#include "procfs.h"
#include "kmalloc.h"
#include "errno.h"
#include "print.h"
#include "memory.h"
#include "scheduler.h"
#include "math.h"

/* private */

static void uint_to_str(unsigned int num, char *buffer, int buffer_size, int base) {
	const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i = buffer_size - 1;
	buffer[i] = '\0'; // Null-terminate the buffer
	i --;

	if (num == 0) {
		buffer[0] = '0';
		buffer[1] = 0;
		return;
	}

	// Convert the number to the specified base
	while (num > 0 && i >= 0) {
		buffer[i] = digits[num % base];
		num /= base;
		i --;
	}

	// Check if the buffer was big enough
	if (num > 0) {
		buffer[0] = '\0'; // Indicate failure if the buffer is too small
		return;
	}

	// Shift the result to the start of the buffer
	int j = 0;
	++i;
	while (buffer[i] != '\0' && j < buffer_size - 1) {
		buffer[j++] = buffer[i++];
	}
	buffer[j] = '\0';
}

static unsigned int str_to_uint(const char* str, int base) {
	unsigned int result = 0;
	while (*str) {
		char c = *str;
		int value;

		if (c >= '0' && c <= '9') {
			value = c - '0';
		} else if (c >= 'A' && c <= 'Z') {
			value = c - 'A' + 10;
		} else {
			break;
		}

		if (value >= base) {
			break; // Invalid digit for the given base
		}

		result = result * base + value;
		str ++;
	}

	return result;
}

/*
 * PROC_ROOT -> PROC_PROC
 */
typedef enum {
	PROC_NODE_ROOT, /* /         */
	PROC_NODE_PROC, /* /$pid/    */
	PROC_NODE_FD,   /* /$pid/fd/ */

	PROC_LEAF_EXE,  /* /$pid/exe */
	PROC_LEAF_CWD,  /* /$pid/cwd */
	PROC_LEAF_SELF, /* /self     */
} ProcNode;

typedef struct {

	// the process id, for nodes after and including PROC
	int pid;

	// current procfs node
	ProcNode node;

	int offset;

} ProcState;

static int proc_sread(void* output_buffer, int output_size, void* file_buffer, int file_size, vRef* vref) {
	ProcState* state = (ProcState*) vref->state;
	int offset = state->offset;

	if (offset < 0) {
		offset = 0;
	}

	int left = file_size - offset;
	int bytes = min(left, output_size);

	memcpy(output_buffer, file_buffer + offset, bytes);

	state->offset = offset + bytes;
	return bytes;
}

/* exported */

int procfs_root(vRef* dst) {
	kprintf("procfs: root\n");
	ProcState* state = kmalloc(sizeof(ProcState));
	state->pid = 0;
	state->offset = 0;
	state->node = PROC_NODE_ROOT;
	dst->state = state;
	return 0;
}

int procfs_clone(vRef* dst, vRef* src) {
	kprintf("procfs: clone\n");
	dst->state = kmalloc(sizeof(ProcState));
	memcpy(dst->state, src->state, sizeof(ProcState));
	return 0;
}

int procfs_open(vRef* vref, const char* basename, uint32_t flags) {
	kprintf("procfs: open %s\n", basename);
	ProcState* state = vref->state;

	// in the root only active PIDs and 'self' are valid
	if (state->node == PROC_NODE_ROOT) {

		if (streq(basename, "self")) {
			state->pid = scheduler_get_current_pid();
			state->offset = 0;

			if (flags & OPEN_NOFOLLOW) {
				state->node = PROC_LEAF_SELF;
			} else {
				state->node = PROC_NODE_PROC;
			}

			return 0;
		}

		int expected = str_to_uint(basename, 10);
		int pid = 0;

		while (scheduler_process_list(&pid)) {
			if (expected == pid) {
				state->offset = 0;
				state->pid = expected;
				state->node = PROC_NODE_PROC;
				return 0;
			}
		}

		return -LINUX_ENOENT;
	}

	if (state->node == PROC_NODE_PROC) {

		if (streq(basename, "..")) {
			state->node = PROC_NODE_ROOT;
			state->offset = 0;
			return 0;
		}

		if (streq(basename, "fd")) {
			state->offset = 0;

			if (flags & OPEN_NOFOLLOW) {
				state->node = PROC_NODE_FD;
			} else {
				// TODO follow link
			}

			return 0;
		}

		if (streq(basename, "exe")) {
			state->offset = 0;

			if (flags & OPEN_NOFOLLOW) {
				state->node = PROC_LEAF_EXE;
			} else {
				// TODO follow link
			}

			return 0;
		}

		// This is link and thus this is highly incorrect
		if (streq(basename, "cwd")) {
			state->offset = 0;
			state->node = PROC_LEAF_CWD;
			return 0;
		}

		return -LINUX_ENOENT;
	}

	if (state->node == PROC_NODE_FD) {

		if (streq(basename, "..")) {
			state->offset = 0;
			state->node = PROC_NODE_PROC;
			return 0;
		}

		return -LINUX_ENOENT;
	}

	return -LINUX_EIO;
}

int procfs_close(vRef* vref) {
	kprintf("procfs: close\n");
	kfree(vref->state);
	return 0;
}

int procfs_read(vRef* vref, void* buffer, uint32_t size) {
	kprintf("procfs: read %d\n", size);
	ProcState* state = vref->state;

	if (state->node == PROC_LEAF_CWD) {
		ProcessDescriptor process;
		scheduler_load_process_info(&process, state->pid);
		vfs_trace(&process.cwd, buffer, size);
		return 0;
	}

	if (state->node == PROC_LEAF_EXE) {
		ProcessDescriptor process;
		scheduler_load_process_info(&process, state->pid);
		vfs_trace(&process.exe, buffer, size);
		return 0;
	}

	if (state->node == PROC_LEAF_SELF) {
		char tmp[16];
		uint_to_str(state->pid, tmp, 16, 10);
		return proc_sread(buffer, size, tmp, 16, vref);
	}

	return -LINUX_EINVAL;
}

int procfs_write(vRef* vref, void* buffer, uint32_t size) {
	kprintf("procfs: write %d\n", size);

	// ignore arguments
	(void) vref;
	(void) buffer;
	(void) size;

	return -LINUX_EROFS; // never allow writing to ProcFS
}

int procfs_seek(vRef* vref, int offset, int whence) {
	kprintf("procfs: seek %d\n", offset);
	ProcState* state = vref->state;

	if (whence == SEEK_SET) {
		state->offset = offset;
		return state->offset;
	}

	if (whence == SEEK_CUR) {
		state->offset += offset;
		return state->offset;
	}

	return -LINUX_EINVAL;
}

int procfs_list(vRef* vref, vEntry* buffer, int max) {
	kprintf("procfs: list\n");
	ProcState* state = vref->state;

	if (state->node == PROC_NODE_ROOT) {
		int size = 3; // ., .., self
		int i = 3;

		int pid = 0;
		while (scheduler_process_list(&pid)) {
			size ++;
		}

		vEntry* entries = kmalloc(sizeof(vEntry) * size);

		memcpy(entries[0].name, ".", 2);
		entries[0].type = DT_DIR;
		entries[0].name_length = 2;

		memcpy(entries[1].name, "..", 3);
		entries[1].type = DT_DIR;
		entries[1].name_length = 3;

		memcpy(entries[2].name, "self", 5);
		entries[2].type = DT_LNK;
		entries[2].name_length = 5;

		pid = 0;
		while (scheduler_process_list(&pid)) {

			char buffer[16];
			uint_to_str(pid, buffer, 16, 10);

			int length = strlen(buffer);
			memcpy(entries[i].name, buffer, length + 1);
			entries[i].type = DT_DIR;
			entries[i].name_length = length;

			i ++;
		}

		for (int i = 0; i < size; i ++) {
			entries[i].seek_offset = i * sizeof(vEntry);
		}

		int bytes = proc_sread(buffer, max * sizeof(vEntry), entries, size * sizeof(vEntry), vref);
		kfree(entries);
		return bytes / sizeof(vEntry);
	}

	if (state->node == PROC_NODE_PROC) {
		vEntry entries[5] = {
			{.seek_offset = sizeof(vEntry) * 0, .name_length = 1, .type=DT_DIR}, // .
			{.seek_offset = sizeof(vEntry) * 1, .name_length = 2, .type=DT_DIR}, // ..
			{.seek_offset = sizeof(vEntry) * 2, .name_length = 3, .type=DT_LNK}, // exe
			{.seek_offset = sizeof(vEntry) * 3, .name_length = 3, .type=DT_LNK}, // cwd
			{.seek_offset = sizeof(vEntry) * 4, .name_length = 2, .type=DT_DIR}, // fd
		};

		memcpy(entries[0].name, ".", 2);
		memcpy(entries[1].name, "..", 3);
		memcpy(entries[2].name, "exe", 4);
		memcpy(entries[3].name, "cwd", 4);
		memcpy(entries[4].name, "fd", 3);

		return proc_sread(buffer, max * sizeof(vEntry), entries, 5 * sizeof(vEntry), vref) / sizeof(vEntry);
	}

	if (state->node == PROC_NODE_FD) {
		vEntry entries[2] = {
			{.seek_offset = sizeof(vEntry) * 0, .name_length = 1, .type=DT_DIR}, // .
			{.seek_offset = sizeof(vEntry) * 1, .name_length = 2, .type=DT_DIR}, // ..
		};

		memcpy(entries[0].name, ".", 2);
		memcpy(entries[1].name, "..", 3);

		return proc_sread(buffer, max * sizeof(vEntry), entries, sizeof(entries), vref) / sizeof(vEntry);
	}

	return LINUX_EIO;
}

int procfs_mkdir(vRef* vref, const char* name) {
	kprintf("procfs: mkdir %s\n", name);

	// ignore arguments
	(void) vref;
	(void) name;

	return -LINUX_EROFS; // never allow writing to ProcFS
}

int procfs_remove(vRef* vref, bool rmdir) {
	kprintf("procfs: remove\n");

	// ignore arguments
	(void) vref;
	(void) rmdir;

	return -LINUX_EROFS; // never allow writing to ProcFS
}

int procfs_stat(vRef* vref, vStat* stat) {
	kprintf("procfs: stat\n");
	ProcState* state = vref->state;

	stat->atime = 0;
	stat->mtime = 0;
	stat->blocks = 0;
	stat->writtable = false;
	stat->size = 1024; // lie :)

	if (state->node == PROC_NODE_ROOT) {
		stat->type = DT_DIR;
		return 0;
	}

	if (state->node == PROC_NODE_PROC) {
		stat->type = DT_DIR;
		return 0;
	}

	if (state->node == PROC_NODE_FD) {
		stat->type = DT_DIR;
		return 0;
	}

	if (state->node == PROC_LEAF_CWD) {
		stat->type = DT_LNK;
		return 0;
	}

	if (state->node == PROC_LEAF_EXE) {
		stat->type = DT_LNK;
		return 0;
	}

	if (state->node == PROC_LEAF_SELF) {
		stat->type = DT_LNK;
		return 0;
	}

	return -LINUX_EIO;
}

int procfs_readlink(vRef* vref, const char* name, char* buffer, int size) {
	kprintf("procfs: readlink\n");
	ProcState* state = vref->state;

	if (state->node == PROC_NODE_PROC && streq(name, "cwd")) {
		ProcessDescriptor process;
		scheduler_load_process_info(&process, state->pid);
		vfs_trace(&process.cwd, buffer, size);
		return 0;
	}

	if (state->node == PROC_NODE_PROC && streq(name, "exe")) {
		ProcessDescriptor process;
		scheduler_load_process_info(&process, state->pid);
		vfs_trace(&process.exe, buffer, size);
		return 0;
	}

	if (state->node == PROC_NODE_ROOT && streq(name, "self")) {
		char file_buffer[16];
		int caller = scheduler_get_current_pid();
		uint_to_str(caller, file_buffer, 16, 10);
		int length = strlen(file_buffer) + 1;

		return proc_sread(buffer, size, file_buffer, length, vref);
	}

	return -LINUX_EINVAL;
}

int procfs_lookup(vRef* vref, char* name) {
	kprintf("procfs: lookup\n");
	ProcState* state = vref->state;

	if (state->node == PROC_NODE_PROC) {
		uint_to_str(state->pid, name, 16, 10);
		return 0;
	}

	if (state->node == PROC_NODE_FD) {
		memcpy(name, "fd", 3);
		return 0;
	}

	if (state->node == PROC_LEAF_CWD) {
		memcpy(name, "cwd", 4);
		return 0;
	}

	if (state->node == PROC_LEAF_EXE) {
		memcpy(name, "exe", 4);
		return 0;
	}

	if (state->node == PROC_LEAF_SELF) {
		memcpy(name, "self", 5);
		return 0;
	}

	return -1;
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
	driver->lookup = procfs_lookup;
}
