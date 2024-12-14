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
