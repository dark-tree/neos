#pragma once

#include "types.h"

/**
 * @brief Writes a byte to a port.
 * 
 * @param port The port to write to.
 * @param data The byte to write.
 * 
 * @return None.
 */
void outb(uint16_t port, uint8_t data);

/**
 * @brief Reads a byte from a port.
 * 
 * @param port The port to read from.
 * 
 * @return The byte read from the port.
 */
uint8_t inb(uint16_t port);
