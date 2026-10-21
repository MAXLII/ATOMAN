// SPDX-License-Identifier: MIT
/**
 * @file platform_port.c
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
#include "esp_timer.h"
#include "esp_system.h"
#include "perf.h"
#include "section.h"

static volatile uint32_t perf_counter_us; /**< Task-owned 1 MHz shadow, wraps after about 71 min. */

uint32_t platform_tick_100us_get(void)
{
    return (uint32_t)((uint64_t)esp_timer_get_time() / 100u);
}
void platform_perf_counter_refresh(void)
{
    perf_counter_us = (uint32_t)(uint64_t)esp_timer_get_time();
}
void platform_system_reset(void)
{
    esp_restart();
}
REG_PERF_BASE_CNT(&perf_counter_us, 1.0e-6f)
