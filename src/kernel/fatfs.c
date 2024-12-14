#include "fatfs.h"
#include "fat.h"
#include "floppy.h"
#include "kmalloc.h"
#include "errno.h"
#include "print.h"
#include "memory.h"
#include "util.h"

/* private */

void read_func(unsigned char* data_out, unsigned int offset_in, unsigned int size_in, void* user_args) {
	floppy_read(data_out, offset_in, size_in);
}

void write_func(unsigned char* data_in, unsigned int offset_in, unsigned int size_in, void* user_args) {
	floppy_write(data_in, offset_in, size_in, true);
}

fat_DISK disk;

typedef struct state_data_s {
	bool is_dir;
	union {
		fat_DIR dir;
		fat_FILE file;
	};
} state_data;

/* exported */

int fatfs_root(vRef* dst) {
	kprintf("fatfs: root\n");

	if (!floppy_init()){
		kprintf("fatfs: floppy init fail\n");
		return LINUX_EIO;
	}

	if (fat_init(&disk, read_func, write_func, 0)) {
		state_data* state = kmalloc(sizeof(state_data));
		dst->state = state;
		state->is_dir = true;
		fat_copy_DIR(&state->dir, &disk.root_directory);
		//kprintf("fatfs: root success\n");
		return 0;
	}
	kprintf("fatfs: root fail\n");
	return LINUX_EIO;
}

int fatfs_clone(vRef* dst, vRef* src) {
	kprintf("fatfs: clone\n");

	dst->state = kmalloc(sizeof(state_data));
	memcpy(dst->state, src->state, sizeof(state_data));

	return 0;
}

int fatfs_open(vRef* vref, const char* basename, uint32_t flags) {
	kprintf("fatfs: open %s\n", basename);

	state_data* state = vref->state;

	if (state == NULL || !state->is_dir) {
		return LINUX_EIO;
	}

	fat_DIR parent_dir = state->dir;

	if (flags & OPEN_DIRECTORY) {
		if (fat_opendir(&state->dir, &parent_dir, basename)) {
			state->is_dir = true;
			kprintf("fatfs: dopen success\n");
			return 0;
		}
		// TODO: report ENOTDIR if the file is not a directory and open failed
	}
	else {
		const char* mode = (flags & OPEN_APPEND) ? "a" : (flags & OPEN_CREAT) ? "w" : "r";
		if (fat_fopen(&state->file, &parent_dir, basename, mode)) {
			state->is_dir = false;
			kprintf("fatfs: fopen success\n");
			return 0;
		}
		// TODO: report EEXIST if the file exists 
	}

	return LINUX_EIO;
}

int fatfs_close(vRef* vref) {
	kprintf("fatfs: close\n");
	kfree(vref->state);
	return 0;
}

int fatfs_read(vRef* vref, void* buffer, uint32_t size) {
	kprintf("fatfs: read %d\n", size);

	state_data* state = vref->state;

	if (state->is_dir) {
		return LINUX_EISDIR;
	}
	else {
		fat_FILE* file = &state->file;
		if (fat_fread(buffer, 1, size, file)) {
			return 0;
		}
	}

	return LINUX_EIO;
}

int fatfs_write(vRef* vref, void* buffer, uint32_t size) {
	kprintf("fatfs: write %d\n", size);

	state_data* state = vref->state;

	if (state->is_dir) {
		return LINUX_EISDIR;
	}
	else {
		fat_FILE* file = &state->file;
		if (fat_fwrite(buffer, 1, size, file)) {
			return 0;
		}
	}

	return LINUX_EIO;
}

int fatfs_seek(vRef* vref, int offset, int whence) {
	kprintf("fatfs: seek %d\n", offset);

	state_data* state = vref->state;

	// TODO: fat seek functions do not perform proper validation
	if (state->is_dir) {
		fat_DIR* dir = &state->dir;
		if (fat_dirseek(dir, offset, whence)) {
			return dir->dir_file.cursor;
		}
	}
	else {
		fat_FILE* file = &state->file;
		if (fat_fseek(file, offset, whence)) {
			return file->cursor;
		}
	}
	return LINUX_EIO;
}

int fatfs_list(vRef* vref, vEntry* entries, int max) {
	kprintf("fatfs: list\n");

	state_data* state = vref->state;

	if (state->is_dir) {
		fat_DIR* dir = &state->dir;
		fat_rewinddir(dir);
		int i = 0;
		fat_DIR* dir_entry;
		fat_FILE* file_entry;
		for (int i = 0; i < max; i++) {
			int result = fat_readdir(dir_entry, file_entry, dir);
			if (result != fat_NOT_FOUND){
				fat_FILE* file_representation = (result == fat_FOUND_FILE) ? file_entry : &dir_entry->dir_file;
				entries[i].seek_offset = dir_entry->dir_file.entry_position;
				entries[i].name_length = fat_longname_to_string(file_representation->long_filename, entries[i].name);
				entries[i].type = (result == fat_FOUND_FILE) ? DT_REG : DT_DIR;
			}
			else {
				break;
			}
		}
		return 0;
	}

	return LINUX_EIO;
}

int fatfs_mkdir(vRef* vref, const char* name) {
	kprintf("fatfs: mkdir %s\n", name);

	state_data* state = vref->state;

	fat_DIR* parent_dir;
	if (state->is_dir) {
		parent_dir = &state->dir;
	}
	else {
		return LINUX_ENOTDIR;
	}

	fat_DIR new_dir;
	if (fat_create_dir(&new_dir, parent_dir, name, 0)) {
		return 0;
	}

	return LINUX_EROFS;
}

int fatfs_remove(vRef* vref, bool rmdir) {
	kprintf("fatfs: remove\n");

	// TODO: non-recursive remove

	state_data* state = vref->state;

	if (state->is_dir) {
		if (rmdir) {
			if (fat_dirremove(&state->dir)) {
				return 0;
			}
		}
		else {
			return LINUX_EISDIR;
		}
	}
	else {
		if (fat_fremove(&state->file)) {
			return 0;
		}
	}

	return LINUX_EIO;
}

int fatfs_stat(vRef* vref, vStat* stat) {
	kprintf("fatfs: stat\n");

	state_data* state = vref->state;
	fat_FILE* file = (state->is_dir) ? &state->dir.dir_file : &state->file;

	stat->size = file->fat_dir.DIR_FileSize;
	stat->atime = 0;
	stat->mtime = 0;
	stat->blocks = (stat->size + file->disk->bpb.BPB_BytsPerSec - 1) / file->disk->bpb.BPB_BytsPerSec;
	stat->writtable = !(file->fat_dir.DIR_Attr & fat_ATTR_READ_ONLY);
	stat->type = (state->is_dir) ? DT_DIR : DT_REG;

	return 0;
}

int fatfs_readlink(vRef* vref, const char* name, const char* buffer, int size) {
	kprintf("fatfs: readlink\n");
	return LINUX_EIO; // TODO
}

/* public */

void fatfs_load(FilesystemDriver* driver) {
	memcpy(driver->identifier, "FatFS", 5);

	// export driver functions
	driver->root = fatfs_root;
	driver->clone = fatfs_clone;
	driver->open = fatfs_open;
	driver->close = fatfs_close;
	driver->read = fatfs_read;
	driver->write = fatfs_write;
	driver->seek = fatfs_seek;
	driver->list = fatfs_list;
	driver->mkdir = fatfs_mkdir;
	driver->remove = fatfs_remove;
	driver->stat = fatfs_stat;
	driver->readlink = fatfs_readlink;
}
