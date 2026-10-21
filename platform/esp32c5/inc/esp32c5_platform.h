// SPDX-License-Identifier: MIT
/**
 * @file esp32c5_platform.h
 * @brief Task-owned SECTION time, reset and transport bindings.
 * @details Part of base. The IDF main task exclusively runs the baremetal SECTION runtime.
 * @author Max.Li
 * @date 2026-10-21
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#ifndef ESP32C5_PLATFORM_H
#define ESP32C5_PLATFORM_H
#include <stdint.h>
/** @return Low 32 bits of the monotonic 100 us timebase. */
uint32_t platform_tick_100us_get(void);
/** @brief Refresh the 1 MHz Perf shadow immediately before a measurement. */
void platform_perf_counter_refresh(void);
/** @brief Reset through ESP-IDF. */
void platform_system_reset(void);
/** @brief Drain bounded TX queues and service TCP session ownership in the main task. */
void esp32c5_link_poll(void);
/** @brief ESP-IDF firmware entry, called on the SDK main task. */
void app_main(void);
#endif /* ESP32C5_PLATFORM_H */
