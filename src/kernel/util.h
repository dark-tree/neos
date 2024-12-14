#pragma once

/**
 * @brief Stop the execution. The processor will not return from this function. Interrupts
 *        will still be processed as long as they are enabled before entring into halt().
 *
 * @return None.
 */
extern void halt();

/**
 * @brief Crashes the kernel with the given error message
 *        and prints some debug information.  This function does not return!
 *
 * @param[in] message C string that will be printed as the error message.
 *
 * @return None.
 */
void panic(const char* message);

/**
 * @brief Prints the values of all registers and 32 dwords from the stack.
 *        Can ba called from ASM like so: `call dump`, preserves ALL registers
 *
 * @return None.
 */
void dump();
