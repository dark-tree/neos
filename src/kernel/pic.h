#pragma once

#include "types.h"

/**
 *
 */
extern void pic_disable();

/**
 *
 */
extern void pic_enable();

/**
 *
 */
extern void pic_remap(uint8_t offset);

/**
 *
 */
extern uint16_t pic_isr();

/**
 *
 */
extern uint16_t pic_irq(int irq);

/**
 *
 */
extern void pic_accept(uint16_t irq_mask);
