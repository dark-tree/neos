#pragma once

/**
 * @brief Stop the execution. The processor will not return from this function. Interrupts
 *        will still be processed as long as they are enabled before entring into halt().
 *
 * @return None.
 */
void halt();
