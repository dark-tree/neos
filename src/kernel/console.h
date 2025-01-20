#pragma once

#include "types.h"

/**
 * @brief Initializes the console subsystem, this needs to be called before the kernel
 *        can print anything to the console. Sets the resultion of the screen.
 *        Cursor is placed at the first line one, and first culumn.
 *
 * @param[in] max_width  Screen columns
 * @param[in] max_height Screen rows
 *
 * @return None.
 */
void con_init(int max_width, int max_height);

/**
 * @brief Scrolls the screen N row up or down, once texts scrolls outside
 *        the screen area it can not be scrolled back to.
 *        Cursor position is updated to move along with the content.
 *
 * @param[in] offset Scroll direction and line count, use to negative values scroll down, and positive to scroll up
 *
 * @return None.
 */
void con_scroll(int offset);

/**
 * @brief Erases the screen content starting at (x1, y1) until (x2, y2),
 *        note that this method dosn't erase a rectangle but a selection.
 *        Cursor position is not updated.
 *
 * @param[in] x1 The start row.
 * @param[in] y1 The start column.
 * @param[in] x2 The end row.
 * @param[in] y2 The end column.
 *
 * @return None.
 */
void con_erase(int x1, int y1, int x2, int y2);

/**
 * @brief Writes a single byte (char) to the system screen console,
 *        this functions handles ANSI control codes. (See ansi.h).
 *        Cursor position is updated to reflect the written commands.
 *
 * @param[in] The byte, or byte sequence part, to be written to the console.
 *
 * @return None.
 */
void con_write(char code);
