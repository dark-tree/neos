#pragma once

#include "types.h"

/**
 * @brief Initializes the console subsystem, this needs to be called before the kernel
 *        can print anything to the console. Sets the resultion of the screen.
 *
 * @param[in] max_width  Screen columns
 * @param[in] max_height Screen rows
 *
 * @return None.
 */
void con_init(int max_width, int max_height);

/**
 * @brief Scrolls the screen one row up or down, once texts scrolls outside
 *        the screen area it can not be scrolled back to.
 *
 * @param[in] offset Scroll direction, -1 to scroll down, +1 to scroll up
 *
 * @return None.
 */
void con_scroll(int offset);

/**
 * @brief Scrolls the screen one row up or down, once texts scrolls outside
 *        the screen area it can not be scrolled back to.
 *
 * @param[in] offset Scroll direction, -1 to scroll down, +1 to scroll up
 *
 * @return None.
 */
void con_erase(int x1, int y1, int x2, int y2);

/**
 * @brief Writes a single byte (char) to the system screen console,
 *        this functions handles ANSI control codes. (See ansi.h).
 *
 * @param[in] The byte, or byte sequence part, to be written to the console.
 *
 * @return None.
 */
void con_write(char code);
