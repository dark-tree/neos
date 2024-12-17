#pragma once

/**
 * @brief Signature of the function that will be invoked by the Interrupt Service Routine.
 *        The values of the registers as they were when the interrupt was triggered are passed this function
 *        unchanged. The first register (EAX) is passed by pointer and can be used in the handler to return a value to the invoker.
 */
typedef void (*interrupt_hander) (int number, int error, int* eax, int ecx, int edx, int ebx, int esi, int edi);

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
 * @param[in] interrupt The interrupt number of the interrupt to handle, see table in routine.asm.
 * @param[in] handler   Pointer to a CDECL function to invoke at the interrupt.
 *
 * @return None.
 */
extern void isr_register(int interrupt, interrupt_hander handler);

/**
 * @brief Translates the interrupt number into a pointer to a human-readable
 *        string pointer.
 *
 * @param[in] interrupt Interrupt number to get the name of.
 *
 * @return String pointer or 0 if no name was found.
 */
extern const char* isr_name(int interrupt);


extern void* isr_stub_stack(void* stack, void* eip, uint32_t data_gdt_index, uint32_t code_gdt_index, int virtual_offset);


extern void isr_into_stack(void* stack);
