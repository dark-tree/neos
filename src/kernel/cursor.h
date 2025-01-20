#pragma once

/**
 * @brief Show/Enable the VGA cursor
 *
 * @return None.
 */
extern void cur_enable();

/**
 * @brief Hide/Disable the VGA cursor
 *
 * @return None.
 */
extern void cur_disable();

/**
 * @brief Move the cursor on the screen
 *
 * @param[in] index The index of the character on screen over which to place the cursor
 *
 * @return None.
 */
extern void cur_goto(int index);
