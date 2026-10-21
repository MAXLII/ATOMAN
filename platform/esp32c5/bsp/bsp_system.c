// SPDX-License-Identifier: MIT
/**
 * @file bsp_system.c
 * @brief ESP-IDF monotonic clock and MCU reset primitives.
 * @details Part of base. No SECTION, Perf registration or application state.
 *          Entity: SDK clock/reset. Prior: initialized SDK. Time: bounded reads.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#include "bsp_system.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_cpu.h"
#include "esp_app_desc.h"
#include <stddef.h>
#include <string.h>

extern uint8_t _data_start[];
extern uint8_t _bss_end[];
uint32_t bsp_system_critical_enter(void)
{
    uint32_t enabled = esp_cpu_intr_get_enabled_mask();
    esp_cpu_intr_disable(UINT32_MAX);
    return enabled;
}
void bsp_system_critical_exit(uint32_t state)
{
    esp_cpu_intr_enable(state);
}
void bsp_system_firmware_identity(uint8_t hash[32])
{
    memcpy(hash, esp_app_get_description()->app_elf_sha256, 32u);
}
const volatile void *bsp_system_sample_address(uint32_t address, uint8_t size)
{
    uintptr_t start = (uintptr_t)_data_start, end = (uintptr_t)_bss_end;

    if (    size == 0u
         || (uintptr_t)address < start
         || (uintptr_t)address >= end
         || (uintptr_t)size > end - (uintptr_t)address
         || address % (size > 4u ? 4u : size) != 0u)
        return NULL;
    return (const volatile void *)(uintptr_t)address;
}

uint32_t bsp_system_tick_100us_get(void)
{
    return (uint32_t)((uint64_t)esp_timer_get_time() / 100u);
}
uint32_t bsp_system_time_us_get(void)
{
    return (uint32_t)(uint64_t)esp_timer_get_time();
}
void bsp_system_reset(void)
{
    esp_restart();
}
