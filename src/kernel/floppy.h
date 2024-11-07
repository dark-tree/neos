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

/**
 * @brief Writes data to the floppy disk.
 * 
 * @param buffer The buffer to write the data from.
 * @param address The address to write to.
 * @param size The size of the data to write.
 * @param preserve Whether to preserve the existing data on the rest of the sector if the data to be written is smaller than the sector size.
 *                 If true, additional reads might be performed to preserve the existing data.
 * 
 * @return true if the data was written successfully, false otherwise.
 */
bool floppy_write(void* buffer, uint32_t address, uint32_t size, bool preserve);
