#pragma once
#include "types.h"


/**
 * @brief Creates new GDT and updates the GDTR register.
 *
 * @return None.
 */
void ginit();

int gput(uint32_t offset, uint32_t size);

void grm(int index);
