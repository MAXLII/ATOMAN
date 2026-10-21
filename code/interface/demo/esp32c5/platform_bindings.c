// SPDX-License-Identifier: MIT
/**
 * @file platform_bindings.c
 * @brief SECTION and Perf bindings for the ESP32-C5 demo.
 * @details Part of base. All framework activity stays on the IDF main task.
 *          Perf measures elapsed callback time including SDK scheduling delays.
 * @author Max.Li
 * @date 2026-10-21
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#include "esp32c5_platform.h"
#include "bsp_system.h"
#include "perf.h"
#include "section.h"

static volatile uint32_t perf_counter_us; /**< Task-owned 1 MHz shadow, wraps after about 71 min. */

uint32_t platform_tick_100us_get(void)
{
    return bsp_system_tick_100us_get();
}
void platform_perf_counter_refresh(void)
{
    perf_counter_us = bsp_system_time_us_get();
}
void platform_system_reset(void)
{
    bsp_system_reset();
}
REG_PERF_BASE_CNT(&perf_counter_us, 1.0e-6f)
