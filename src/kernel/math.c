
#include "math.h"

/* public */

int max(int left, int right) {
	return (left > right) ? left : right;
}

int min(int left, int right) {
	return (left < right) ? left : right;
}

int clamp(int value, int low, int high) {
	if (value < low) return low;
	if (value > high) return high;

	return value;
}

int sgn(int value) {
	if (value < 0) return -1;
	if (value > 0) return +1;

	return 0;
}

int abs(int value) {
	return (value < 0) ? (-value) : (+value);
}
