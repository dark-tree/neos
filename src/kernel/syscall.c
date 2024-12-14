#include "syscall.h"
#include "print.h"
#include "types.h"
#include "util.h"
#include "errno.h"
#include "scheduler.h"

/* private */

struct SyscallEntry_tag;

typedef int (*syscall_fn0) ();
typedef int (*syscall_fn1) (int);
typedef int (*syscall_fn2) (int, int);
typedef int (*syscall_fn3) (int, int, int);
typedef int (*syscall_fn4) (int, int, int, int);
typedef int (*syscall_fn5) (int, int, int, int, int);
typedef int (*syscall_adapt) (struct SyscallEntry_tag* entry, int, int, int, int, int);

typedef struct SyscallEntry_tag {
	syscall_adapt adapter;
	void* handler;
} SyscallEntry;

static int syscall_adapter_fn0(struct SyscallEntry_tag* entry, int, int, int, int, int) {
	if (entry->handler) {
		return ((syscall_fn0) entry->handler) ();
	}

	panic("Unimplemented Linux syscall invoked!");
	return -1;
}

static int syscall_adapter_fn1(struct SyscallEntry_tag* entry, int ebx, int, int, int, int) {
	if (entry->handler) {
		return ((syscall_fn1) entry->handler) (ebx);
	}

	panic("Unimplemented Linux syscall invoked!");
	return -1;
}

static int syscall_adapter_fn2(struct SyscallEntry_tag* entry, int ebx, int ecx, int, int, int) {
	if (entry->handler) {
		return ((syscall_fn2) entry->handler) (ebx, ecx);
	}

	panic("Unimplemented Linux syscall invoked!");
	return -1;
}

static int syscall_adapter_fn3(struct SyscallEntry_tag* entry, int ebx, int ecx, int edx, int, int) {
	if (entry->handler) {
		return ((syscall_fn3) entry->handler) (ebx, ecx, edx);
	}

	panic("Unimplemented Linux syscall invoked!");
	return -1;
}

static int syscall_adapter_fn4(struct SyscallEntry_tag* entry, int ebx, int ecx, int edx, int esi, int) {
	if (entry->handler) {
		return ((syscall_fn4) entry->handler) (ebx, ecx, edx, esi);
	}

	panic("Unimplemented Linux syscall invoked!");
	return -1;
}

static int syscall_adapter_fn5(struct SyscallEntry_tag* entry, int ebx, int ecx, int edx, int esi, int edi) {
	if (entry->handler) {
		return ((syscall_fn5) entry->handler) (ebx, ecx, edx, esi, edi);
	}

	panic("Unimplemented Linux syscall invoked!");
	return -1;
}

/* syscall ustils */

static vRef* fd_resolve(int fd) {

	if (fd <= 0) {
		return NULL;
	}

	int caller = scheduler_get_current_pid();
	return scheduler_fget(caller, fd);

}

static vRef fd_cwd() {

	ProcessDescriptor process;

	// load calling process
	int caller = scheduler_get_current_pid();
	scheduler_load_process_info(&process, caller);

	return process.cwd;

}

static int fd_open(vRef* parent, const char* filename, int flags) {

	vRef new_file;
	int res = vfs_open(&new_file, parent, filename, flags);

	if (res < 0) {
		return res;
	}

	int caller = scheduler_get_current_pid();
	int fd = scheduler_fput(new_file, caller);

	// scheduler_fput uses 0 as error code...
	if (fd == 0) {
		return -LINUX_EMFILE;
	}

	return fd;
}

struct old_linux_dirent {
	unsigned long      d_ino;         /* inode number */
	unsigned long      d_offset;      /* offset to this old_linux_dirent */
	unsigned short     d_namlen;      /* length of this d_name */
	char               d_name[];      /* filename (null-terminated) */
};

struct linux_dirent {
	unsigned long      d_ino;         /* Inode number */
	unsigned long      d_off;         /* Not an offset; see below */
	unsigned short     d_reclen;      /* Length of this linux_dirent */
	char               d_name[];      /* Filename (null-terminated) */
                                      /* length is actually (d_reclen - 2 -
                                         offsetof(struct linux_dirent, d_name)) */

    /* those fields DO exist here but are non-explicit */
	/*char             pad;*/         /* Zero padding byte */
	/*char             d_type;*/      /* File type (only since Linux 2.6.4); offset is (d_reclen - 1) */
};

struct linux_dirent64 {
	uint64_t           d_ino;         /* 64-bit inode number */
	uint64_t           d_off;         /* Not an offset; see getdents() */
	unsigned short     d_reclen;      /* Size of this dirent */
	unsigned char      d_type;        /* File type */
	char               d_name[];      /* Filename (null-terminated) */
};

unsigned short mkmode(vEntryType type, int owner, int group, int other, bool suid, bool sgid) {
	return (type << 4) | owner | (group << 3) | (other << 6) | (suid ? 0x0800 : 0) | (sgid ? 0x0400 : 0);
}

struct __old_kernel_stat {
	unsigned short     st_dev;        /* Device */
	unsigned short     st_ino;        /* File serial number */
	unsigned short     st_mode;       /* File mode */
	unsigned short     st_nlink;      /* Link count */
	unsigned short     st_uid;        /* User ID of the file's owner */
	unsigned short     st_gid;        /* Group ID of the file's group */
	unsigned short     st_rdev;       /* Device number, if device */
	unsigned long      st_size;
	unsigned long      st_atime;      /* Time of last access */
	unsigned long      st_mtime;      /* Time of last modification */
	unsigned long      st_ctime;      /* Time of last status change */
};

static void vstat_to_linux_old(struct __old_kernel_stat* lstat, vStat* vstat) {

	int perm = vstat->writtable ? 7 : 5;

	lstat->st_dev = 1; // driver major/minor
	lstat->st_ino = 0;
	lstat->st_mode = mkmode(vstat->type, perm, perm, perm, false, false);
	lstat->st_nlink = 0;
	lstat->st_uid = 0;
	lstat->st_gid = 0;
	lstat->st_rdev = 1; // major/minor device numbers for special files

	lstat->st_size = vstat->size;
	lstat->st_atime = vstat->atime;
	lstat->st_mtime = vstat->mtime;
	lstat->st_ctime = vstat->mtime;

}

struct stat {
	unsigned long      st_dev;        /* Device */
	unsigned long      st_ino;        /* File serial number */
	unsigned int       st_mode;       /* File mode */
	unsigned int       st_nlink;      /* Link count */
	unsigned int       st_uid;        /* User ID of the file's owner */
	unsigned int       st_gid;        /* Group ID of the file's group */
	unsigned long      st_rdev;       /* Device number, if device */
	unsigned long      __pad1;
	long               st_size;       /* Size of file, in bytes */
	int                st_blksize;    /* Optimal block size for I/O */
	int                __pad2;
	long               st_blocks;     /* Number 512-byte blocks allocated */
	long               st_atime;      /* Time of last access */
	unsigned long      st_atime_nsec;
	long               st_mtime;      /* Time of last modification */
	unsigned long      st_mtime_nsec;
	long               st_ctime;      /* Time of last status change */
	unsigned long      st_ctime_nsec;
	unsigned int       __unused4;
	unsigned int       __unused5;
};

static void vstat_to_linux_32(struct stat* lstat, vStat* vstat) {

	int perm = vstat->writtable ? 7 : 5;

	lstat->st_dev = 1; // driver major/minor
	lstat->st_ino = 0;
	lstat->st_mode = mkmode(vstat->type, perm, perm, perm, false, false);
	lstat->st_nlink = 0;
	lstat->st_uid = 0;
	lstat->st_gid = 0;
	lstat->st_rdev = 1; // major/minor device numbers for special files

	// size
	lstat->st_size = vstat->size;
	lstat->st_blksize = 512;
	lstat->st_blocks = vstat->blocks;

	// time
	lstat->st_atime = vstat->atime;
	lstat->st_atime_nsec = 0;
	lstat->st_mtime = vstat->mtime;
	lstat->st_mtime_nsec = 0;
	lstat->st_ctime = vstat->mtime;
	lstat->st_ctime_nsec = 0;

}

struct stat64 {
	unsigned long long st_dev;        /* Device */
	unsigned long long st_ino;        /* File serial number */
	unsigned           st_mode;       /* File mode */
	unsigned int       st_nlink;      /* Link count */
	unsigned int       st_uid;        /* User ID of the file's owner */
	unsigned int       st_gid;        /* Group ID of the file's group */
	unsigned long long st_rdev;       /* Device number, if device  */
	unsigned long long __pad1;
	long long          st_size;       /* Size of file, in bytes */
	int                st_blksize;    /* Optimal block size for I/O */
	int                __pad2;
	long long          st_blocks;     /* Number 512-byte blocks allocated */
	int                st_atime;      /* Time of last access */
	unsigned int       st_atime_nsec;
	int                st_mtime;      /* Time of last modification */
	unsigned int       st_mtime_nsec;
	int                st_ctime;      /* Time of last status change */
	unsigned int       st_ctime_nsec;
	unsigned int       __unused4;
	unsigned int       __unused5;
};

static void vstat_to_linux_64(struct stat64* lstat, vStat* vstat) {

	int perm = vstat->writtable ? 7 : 5;

	lstat->st_dev = 1; // driver major/minor
	lstat->st_ino = 0;
	lstat->st_mode = mkmode(vstat->type, perm, perm, perm, false, false);
	lstat->st_nlink = 0;
	lstat->st_uid = 0;
	lstat->st_gid = 0;
	lstat->st_rdev = 1; // major/minor device numbers for special files

	// size
	lstat->st_size = vstat->size;
	lstat->st_blksize = 512;
	lstat->st_blocks = vstat->blocks;

	// time
	lstat->st_atime = vstat->atime;
	lstat->st_atime_nsec = 0;
	lstat->st_mtime = vstat->mtime;
	lstat->st_mtime_nsec = 0;
	lstat->st_ctime = vstat->mtime;
	lstat->st_ctime_nsec = 0;

}

static int stat(const char* filename, void* statbuf, void (*converter) (void* dst, vStat* src), int flags) {

	vRef cwd = fd_cwd();
	vRef vref;
	vStat stat;
	int res = 0;

	if (res = vfs_open(&vref, &cwd, filename, flags)) {
		return res;
	}

	if (res = vfs_stat(&vref, &stat)) {
		return res;
	}

	if (res = vfs_close(&vref)) {
		return res;
	}

	converter(statbuf, &stat);
	return 0;

}

static int fstat(unsigned int fd, void* statbuf, void (*converter) (void* dst, vStat* src)) {

	vRef* vref = fd_resolve(fd);
	vStat stat;
	int res = 0;

	if (!vref) {
		return -LINUX_EBADF;
	}

	if (res = vfs_stat(&vref, &stat)) {
		return res;
	}

	converter(statbuf, &stat);
	return 0;
}

/* input/output */

static int sys_write(int fd, char* buffer, int bytes) {

	vRef* vref = fd_resolve(fd);

	if (!vref) {
		return -LINUX_EBADF;
	}

	return vfs_write(vref, buffer, bytes);
}

static int sys_read(int fd, char* buffer, int bytes) {

	vRef* vref = fd_resolve(fd);

	if (!vref) {
		return -LINUX_EBADF;
	}

	return vfs_read(vref, buffer, bytes);
}

static int sys_open(const char* filename, int flags, int mode) {

	vRef cwd = fd_cwd();

	// mode is ignored by our glorious NEOS kernel, who needs permissions anyway?
	(void) mode;

	return fd_open(&cwd, filename, flags);
}

static int sys_openat(int fd, const char* filename, int flags, int mode) {

	vRef* vref = fd_resolve(fd);

	if (!vref) {
		return -LINUX_EBADF;
	}

	// mode is ignored by our glorious NEOS kernel, who needs permissions anyway?
	(void) mode;

	return fd_open(vref, filename, flags);

}

static int sys_creat(const char* pathname, int mode) {
	return sys_open(pathname, OPEN_WRONLY | OPEN_CREAT | OPEN_WRONLY, mode);
}

static int sys_lseek(unsigned int fd, int offset, unsigned int whence) {

	vRef* vref = fd_resolve(fd);

	if (!vref) {
		return -LINUX_EBADF;
	}

	return vfs_seek(vref, offset, whence);
}

static int sys_mkdir(const char* pathname, int mode) {

	vRef cwd = fd_cwd();
	(void) mode;

	return vfs_mkdir(&cwd, pathname);
}

static int sys_readlink(const char* path, char* buf, int size) {

	vRef cwd = fd_cwd();

	return vfs_readlink(&cwd, path, buf, size);
}

static int sys_getdents(unsigned int fd, struct linux_dirent* buffer, unsigned int size) {

	vRef* vref = fd_resolve(fd);

	if (!vref) {
		return -LINUX_EBADF;
	}

	int bytes = 0;

	while (true) {

		vEntry entry;
		int count = vfs_list(vref, &entry, 1);

		// nothing left to list
		if (count == 0) {
			return bytes;
		}

		// check for error
		if (count < 0) {
			return count;
		}

		int esz = sizeof(struct linux_dirent) + entry.name_length + 2;
		int left = size - bytes;

		// check if we run out of space in output buffer
		if (left < esz) {

			// undo last list call
			vfs_seek(vref, entry.seek_offset, SEEK_SET);

			// if we loaded nothing and still run out of space the buffer was too small
			if (bytes == 0) {
				return -LINUX_EINVAL;
			}

			// ... otehrwise return the number of bytes written
			return bytes;
		}

		// next entry pointer
		struct linux_dirent* dirent = ((void*) buffer) + bytes;

		dirent->d_ino = 0; // inode
		dirent->d_off = entry.seek_offset;
		dirent->d_reclen = sizeof(struct linux_dirent) + entry.name_length + 2;

		memcpy(dirent->d_name, entry.name, entry.name_length);

		// extension
		dirent->d_name[entry.name_length + 1] = 0;          // one byte after name (pad)
		dirent->d_name[entry.name_length + 2] = entry.type; // two bytes after name (d_type)

		// now move forward in output buffer
		bytes += dirent->d_reclen;
	}

}

static int sys_getdents64(unsigned int fd, struct linux_dirent64* buffer, unsigned int size) {

	vRef* vref = fd_resolve(fd);

	if (!vref) {
		return -LINUX_EBADF;
	}

	int bytes = 0;

	while (true) {

		vEntry entry;
		int count = vfs_list(vref, &entry, 1);

		// nothing left to list
		if (count == 0) {
			return bytes;
		}

		// check for error
		if (count < 0) {
			return count;
		}

		int esz = sizeof(struct linux_dirent64) + entry.name_length;
		int left = size - bytes;

		// check if we run out of space in output buffer
		if (left < esz) {

			// undo last list call
			vfs_seek(vref, entry.seek_offset, SEEK_SET);

			// if we loaded nothing and still run out of space the buffer was too small
			if (bytes == 0) {
				return -LINUX_EINVAL;
			}

			// ... otehrwise return the number of bytes written
			return bytes;
		}

		// next entry pointer
		struct linux_dirent64* dirent = ((void*) buffer) + bytes;

		dirent->d_ino = 0; // inode
		dirent->d_off = entry.seek_offset;
		dirent->d_reclen = sizeof(struct linux_dirent) + entry.name_length;
		dirent->d_type = entry.type;

		memcpy(dirent->d_name, entry.name, entry.name_length);

		// now move forward in output buffer
		bytes += dirent->d_reclen;
	}

}

static int sys_old_readdir(unsigned int fd, struct old_linux_dirent* buffer, unsigned int size) {

	vRef* vref = fd_resolve(fd);

	if (!vref) {
		return -LINUX_EBADF;
	}

	int bytes = 0;

	while (true) {

		vEntry entry;
		int count = vfs_list(vref, &entry, 1);

		// nothing left to list
		if (count == 0) {
			return bytes;
		}

		// check for error
		if (count < 0) {
			return count;
		}

		int esz = sizeof(struct old_linux_dirent) + entry.name_length;
		int left = size - bytes;

		// check if we run out of space in output buffer
		if (left < esz) {

			// undo last list call
			vfs_seek(vref, entry.seek_offset, SEEK_SET);

			// if we loaded nothing and still run out of space the buffer was too small
			if (bytes == 0) {
				return -LINUX_EINVAL;
			}

			// ... otehrwise return the number of bytes written
			return bytes;
		}

		// next entry pointer
		struct old_linux_dirent* dirent = ((void*) buffer) + bytes;

		dirent->d_ino = 0; // inode
		dirent->d_offset = entry.seek_offset;
		dirent->d_namlen = entry.name_length;

		memcpy(dirent->d_name, entry.name, entry.name_length);

		// now move forward in output buffer
		bytes += sizeof(struct old_linux_dirent);
		bytes += entry.name_length;
	}

}

static int sys_stat(const char* filename, struct __old_kernel_stat* statbuf) {
	return stat(filename, statbuf, vstat_to_linux_old, /* Do follow links */ 0);
}

static int sys_fstat(unsigned int fd, struct __old_kernel_stat* statbuf) {
	return fstat(fd, statbuf, vstat_to_linux_old);
}

static int sys_lstat(const char* filename, struct __old_kernel_stat* statbuf) {
	return stat(filename, statbuf, vstat_to_linux_old, OPEN_NOFOLLOW);
}

static int sys_newstat(const char* filename, struct stat* statbuf) {
	return stat(filename, statbuf, vstat_to_linux_32, /* Do follow links */ 0);
}

static int sys_newlstat(const char* filename, struct stat* statbuf) {
	return stat(filename, statbuf, vstat_to_linux_32, OPEN_NOFOLLOW);
}

static int sys_newfstat(unsigned int fd, struct stat* statbuf) {
	return fstat(fd, statbuf, vstat_to_linux_32);
}

static int sys_stat64(const char* filename, struct stat64* statbuf) {
	return stat(filename, statbuf, vstat_to_linux_64, /* Do follow links */ 0);
}

static int sys_lstat64(const char* filename, struct stat64* statbuf) {
	return stat(filename, statbuf, vstat_to_linux_64, OPEN_NOFOLLOW);
}

static int sys_fstat64(unsigned long fd, struct stat64* statbuf) {
	return fstat(fd, statbuf, vstat_to_linux_64);
}

#define SYSCALL_ENTRY(args, function) {.adapter = syscall_adapter_fn##args, .handler = (void*) (function)}
#define SYSCALL_IMPL
#include "systable.h"

/* public */

int sys_linux(int eax, int ebx, int ecx, int edx, int esi, int edi) {
	if (eax >= SYS_LINUX_SIZE) {
		panic("Invalid syscall number!");
	}

	SyscallEntry* entry = sys_linux_table + eax;
	return entry->adapter(entry, ebx, ecx, edx, esi, edi);
}
