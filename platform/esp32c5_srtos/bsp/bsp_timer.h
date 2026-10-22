// SPDX-License-Identifier: MIT
/**
 * @file bsp_timer.h
 * @brief ESP32-C5 board periodic interrupt without framework dependencies.
 * @details Part of base. One hardware timer owns the fixed 32 us demo timebase.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#ifndef BSP_TIMER_H
#define BSP_TIMER_H
#include <stdint.h>

typedef struct
{
    uint32_t interrupts;      /**< Number of board interrupt callbacks. */
    uint32_t cycle_stamp;     /**< CPU cycle counter captured at ISR entry. */
    uint32_t max_gap_cycles;  /**< Longest observed interval between ISR entries. */
    uint32_t overdue;         /**< Intervals greater than 1.5 configured periods. */
    uint32_t busy_cycles;     /**< Accumulated ISR execution cycles, wrapping modulo 2^32. */
    uint32_t max_body_cycles; /**< Longest execution time of the registered interrupt chain. */
} bsp_timer_debug_t;
extern volatile bsp_timer_debug_t g_bsp_timer_debug; /**< Hardware timing observations for ELF diagnostics. */

/**
 * @brief Start the board timer once, after interrupt consumers are initialized.
 * @param p_callback Bounded ISR callback; must not block or allocate memory.
 * @details Initialization errors abort boot instead of advertising a false rate.
 */
void bsp_timer_init(void (*p_callback)(void));
/** @return Configured interrupt frequency in Hz; 0 before successful startup. */
uint32_t bsp_timer_frequency_get(void);
#endif /* BSP_TIMER_H */
