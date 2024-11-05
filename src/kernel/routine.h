#pragma once

/**
 * @brief Signature of the function that will be invoked by the Interrupt Service Routine
 */
typedef void (*interrupt_hander) (int number, int error);

/**
 * @brief Initializes the contents of the Interrupt Descriptor Table
 *        with valid Interrupt Service Routine pointers. Must be called before enabling CPU interrupts.
 *
 * @param[out] handler Pointer to the Interrupt Service Routine.
 *
 * @return None.
 */
extern void isr_init(void* offset);

/**
 * @brief Register CDECL interrupt handler for the specificed interrupt number,
 *        if an interrupt that has no registered C handler is invoked it will be ignored.
 *
 * @param[in] interrupt The interrupt number of the interrupt to handle, see table in routine.asm
 * @param[in] handler   Pointer to a CDECL function to invoke at the interrupt
 *
 * @return None.
 */
extern void isr_register(int interrupt, interrupt_hander handler);
