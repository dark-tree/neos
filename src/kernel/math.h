#pragma once

/**
 * @brief Returns the larger of the two given values.
 *
 * @param[in]  left  First value.
 * @param[in]  right Second value.
 *
 * @return Returns the larger value.
 */
int max(int left, int right);

/**
 * @brief Returns the smaller of the two given values.
 *
 * @param[in]  left  First value.
 * @param[in]  right Second value.
 *
 * @return Returns the smaller value.
 */
int min(int left, int right);

/**
 * @brief Clamps the given value to the given range.
 *
 * @param[in]  value The value to clamp.
 * @param[in]  low   The lower bound of the value.
 * @param[in]  high  The upper bound of the value.
 *
 * @return Returns a integer in range [low, high].
 */
int clamp(int value, int low, int high);

/**
 * @brief Returns the sign of the number, or 0 if the number is 0,
 *        such that `sgn(val) * abs(val) == val`
 *
 * @param[in] value The value to compute the sign of.
 *
 * @return Returns a integer from set {-1, 0, +1}.
 */
int sgn(int value);

/**
 * @brief Returns the absolute value of the give integer,
 *        such that `abs(-10) == abs(10)`
 *
 * @param[in] value The value to compute the absolute value of.
 *
 * @return Returns a non-negative integer.
 */
int abs(int value);
