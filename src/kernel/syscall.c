#include "syscall.h"
#include "print.h"
#include "types.h"
#include "util.h"

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

int syscall_adapter_fn0(struct SyscallEntry_tag* entry, int, int, int, int, int) {
	if (entry->handler) {
		return ((syscall_fn0) entry->handler) ();
	}

	panic("Unimplemented Linux syscall invoked!");
	return -1;
}

int syscall_adapter_fn1(struct SyscallEntry_tag* entry, int ebx, int, int, int, int) {
	if (entry->handler) {
		return ((syscall_fn1) entry->handler) (ebx);
	}

	panic("Unimplemented Linux syscall invoked!");
	return -1;
}

int syscall_adapter_fn2(struct SyscallEntry_tag* entry, int ebx, int ecx, int, int, int) {
	if (entry->handler) {
		return ((syscall_fn2) entry->handler) (ebx, ecx);
	}

	panic("Unimplemented Linux syscall invoked!");
	return -1;
}

int syscall_adapter_fn3(struct SyscallEntry_tag* entry, int ebx, int ecx, int edx, int, int) {
	if (entry->handler) {
		return ((syscall_fn3) entry->handler) (ebx, ecx, edx);
	}

	panic("Unimplemented Linux syscall invoked!");
	return -1;
}

int syscall_adapter_fn4(struct SyscallEntry_tag* entry, int ebx, int ecx, int edx, int esi, int) {
	if (entry->handler) {
		return ((syscall_fn4) entry->handler) (ebx, ecx, edx, esi);
	}

	panic("Unimplemented Linux syscall invoked!");
	return -1;
}

int syscall_adapter_fn5(struct SyscallEntry_tag* entry, int ebx, int ecx, int edx, int esi, int edi) {
	if (entry->handler) {
		return ((syscall_fn5) entry->handler) (ebx, ecx, edx, esi, edi);
	}

	panic("Unimplemented Linux syscall invoked!");
	return -1;
}

/* input/output */

int sys_write(int fd, char* buffer, int bytes) {
	for (int i = 0; i < bytes; i ++) {
		con_write(buffer[i]);
	}

	return bytes;
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
