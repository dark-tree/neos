#pragma once

#include "types.h"

/**
 * @brief Initializes the floppy controller.
 * 
 * @return true if the floppy controller was initialized successfully, false otherwise.
 */
bool floppy_init();

/**
 * @brief Reads data from the floppy disk.
 * 
 * @param buffer The buffer to read the data into.
 * @param address The address to read from.
 * @param size The size of the data to read.
 * 
 * @return true if the data was read successfully, false otherwise.
 */
bool floppy_read(void* buffer, uint32_t address, uint32_t size);
