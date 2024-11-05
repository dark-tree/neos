#pragma once

/**
 * @brief Initializes the interrupt system and handlers.
 *        This call enables system interrupts.
 *
 * @return None.
 */
void int_init();

/**
 * @brief Configures an interrupt fence for the given interrupt number,
 *        starting with this call the system will look for the specified interrupt.
 *
 * @param[in] interrupt The interrupt number to wait for.
 *
 * @return None.
 */
void int_lock(int interrupt);

/**
 * @brief Will block until the interrupt number specificed by the
 *        previous call to int_lock(), is not detected.
 *
 * @return None.
 */
void int_wait();
