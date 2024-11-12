#pragma once

/**
 * @brief Reconfigures the processor's Global Descriptor Table Register
 *        to point it at the given target address.
 *
 * @param[in] offset Physical address of the start of tables (of the 0th dummy/null entry)
 * @param[in] limit  The number of entries in the tables (not bytes!)
 *
 * @return None.
 */
extern void gdtr_store(int offset, int limit);

/**
 * @brief Reconfigures the processor's Interrupt Descriptor Table Register
 *        to point it at the given target address.
 *
 * @param[in] offset Physical address of the start of tables (of the 0th entry)
 * @param[in] limit  The number of entries in the tables (not bytes!)
 *
 * @return None.
 */
extern void idtr_store(int offset, int limit);

/**
 * @brief Switches code and data segments to point to the given GDT entries,
 *        both code and data indices should be equal or grater one (not point at the dummy GDT entry).
 *
 * @param[in] data Data segment GDT index, must be >= 1.
 * @param[in] code Code segment GDT index, must be >= 1.
 *
 * @return None.
 */
extern void gdtr_switch(int data, int code);
