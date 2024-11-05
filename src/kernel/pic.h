#pragma once

#include "types.h"

/**
 * @brief Disable (mask) all interrupts coming from the Programable Interrupt Controller (PIC),
 *        that is all Hardware Interrupts (IRQs). See routine.asm's ISR table for the IRQ list.
 *
 * @return None.
 */
extern void pic_disable();

/**
 * @brief Enable (unmask) all interrupts coming from the Programable Interrupt Controller (PIC),
 *        that is all Hardware Interrupts (IRQs). See routine.asm's ISR table for the IRQ list.
 *
 * @return None.
 */
extern void pic_enable();

/**
 * @brief Configures the interrupt number at which the IRQs will be "mounted" onto the
 *        interrupt array. Sets the base interrupt number for the following 16 IRQs will be
 *        mapped to the following consecutive interrupt numbers.
 *
 * @param[in] offset The base interrupt number that will be assigned to IRQ0.
 *
 * @return None.
 */
extern void pic_remap(uint8_t offset);

/**
 * @brief Queries the 8 bit ISR ("In-Service Register") from the two PICs and
 *        returns it as a single 16 bit number.
 *
 *        + --------------- + --------------- +
 *        | 15            8 | 7             0 |
 *        + --------------- + --------------- +
 *        |  Slave Bitmask  | Master Bitmask  |
 *        + --------------- + --------------- +
 *
 *        Each bit in this combined register
 *        coresponds to one IRQ like so: `mask = 1 << irq`, so IRQ0 has a mask
 *        of 0x0001, IRQ3 of 0x0004 etc. This value represents the interrupts that
 *        are being handled by the CPU, this value should be checked before processing an IRQ
 *        in order to discard spurious interrupts.
 *
 * @return The PIC's joined 16-bit "In-Service Register"
 */
extern uint16_t pic_isr();

/**
 * @brief Translates the interrupt number into the coresponding IRQ
 *        bitmask, or 0 if the given number is not a IRQ interrupt.
 *
 * @param[in] interrupt The interrupt number to converrt into IRQ bitmask.
 *
 * @return 8 bit IRQ bitmask, or 0 on error.
 */
extern uint16_t pic_irq(int interrupt);

/**
 * @brief Sends the "End Of Interrupt" command to the correct PIC chip
 *        based on whether any bit in the high 8 bits (called slave byte),
 *        and whether any bit in the low 8 bits (called master byte) is set.
 *
 * @param[in] irq_mask 16 bit number, treated as two 8 bit booleans
 *
 * @return None.
 */
extern void pic_accept(uint16_t irq_mask);
