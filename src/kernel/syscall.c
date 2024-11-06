#include "syscall.h"
#include "print.h"

#define SYS_WRITE 0x04 /* write to a file descriptor */

/* private */

int sys_write(int fd, char* buffer, int bytes) {
	for (int i = 0; i < bytes; i ++) {
		con_write(buffer[i]);
	}

	return bytes;
}

/* public */

int sys_linux(int eax, int ebx, int ecx, int edx, int esi, int edi) {

	if (eax == SYS_WRITE) {
		return sys_write(ebx, ecx, edx);
	}

	kprintf("Unknown Linux Syscall 0x%x: ebx=0x%x, ecx=0x%x, edx=0x%x, esi=0x%x, edi=0x%x\n", eax, ebx, ecx, edx, esi, edi);
}
