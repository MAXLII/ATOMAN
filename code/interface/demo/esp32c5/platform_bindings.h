// SPDX-License-Identifier: MIT
/**
 * @file platform_bindings.h
 * @brief SECTION time, reset and interrupt bindings.
 * @details Part of base. The project selects task scheduling; the board timer dispatches interrupts.
 * @author Max.Li
 * @date 2026-10-21
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#ifndef PLATFORM_BINDINGS_H
#define PLATFORM_BINDINGS_H
#include <stdint.h>
/** @return Low 32 bits of the monotonic 100 us timebase. */
uint32_t platform_tick_100us_get(void);
/** @brief Refresh the 1 MHz Perf shadow immediately before a measurement. */
void platform_perf_counter_refresh(void);
/** @brief Reset through ESP-IDF. */
void platform_system_reset(void);
#endif /* PLATFORM_BINDINGS_H */
