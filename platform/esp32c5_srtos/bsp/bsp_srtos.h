// SPDX-License-Identifier: MIT
/**
 * @file bsp_srtos.h
 * @brief ESP-IDF outer-interrupt adapter for a hosted RV32 scheduler.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License.
 */
#ifndef BSP_SRTOS_H
#define BSP_SRTOS_H
#include <stdint.h>
/** @param p_irq Outer interrupt callback receiving integer frame and saved CSRs.
 * @param p_enter Enter a scheduler external-call guard.
 * @param p_exit Leave a scheduler external-call guard. */
void bsp_srtos_bind(void (*p_irq)(uint32_t *, uint32_t, uint32_t), void (*p_enter)(void), void (*p_exit)(void));
/** Publish the interrupt bridge after callbacks and SECTION have been initialized. */
void bsp_srtos_init(void);
/** @param p_frame Original SDK interrupt frame before changing to the ISR stack.
 * @param status Mstatus at trap entry.
 * @param cause Mcause at trap entry, including previous CLIC level. */
void bsp_srtos_irq_enter(uint32_t *p_frame, uint32_t status, uint32_t cause);
/** Process the outer IRQ just before the SDK restores its host context. */
void bsp_srtos_irq_exit(void);
#endif
