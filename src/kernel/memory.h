#pragma once

#include "types.h"

/**
 * @brief Copies `bytes` bytes from `src` to `dst`. The memory blocks must not overlap,
 *        if they do, consider using `memmove()`.
 *
 * @param[out] dst   Pointer to the destination array where the content is to be copied.
 * @param[in]  src   Pointer to the source of data to be copied.
 * @param[in]  bytes Number of bytes to copy.
 *
 * @return Returns a pointer to the buffer that was written to.
 */
void* memcpy (void* dst, const void* src, long bytes);

/**
 * @brief Copies `bytes` bytes from `src` to `dst`. The memory blocks can overlap,
 *        if they do not, consider using `memcpy()`.
 *
 * @param[out] dst   Pointer to the destination array where the content is to be copied.
 * @param[in]  src   Pointer to the source of data to be copied.
 * @param[in]  bytes Number of bytes to copy.
 *
 * @return Returns a pointer to the buffer that was written to.
 */
void* memmove (void* dst, const void* src, long bytes);

/**
 * @brief Sets the first `bytes` bytes of the block of memory
 *        pointed by `dst` to the specified value.
 *
 * @param[out] dst   Pointer to the block of memory to fill.
 * @param[in]  value Value that is to be written to each byte in destination.
 * @param[in]  bytes Number of bytes to be set to the value.
 *
 * @return Returns a pointer to the buffer that was written to.
 */
void* memset (void* dst, uint8_t value, long bytes);
