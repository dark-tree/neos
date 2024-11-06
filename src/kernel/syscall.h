#pragma once

/**
 * @brief Signature of all syscall handlers.
 */
typedef int (*syscall) (int eax, int ebx, int ecx, int edx, int esi, int edi);

/**
 * @brief Invokes a x86 32 bit linux syscall.
 *
 * @param[in] eax The syscall number.
 * @param[in] ... Syscall arguments, refer to a specific syscall for context.
 *
 * @return Returns syscall result that should be placed into EAX when returning from interrupt.
 */
int sys_linux(int eax, int ebx, int ecx, int edx, int esi, int edi);
