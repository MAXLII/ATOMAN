// SPDX-License-Identifier: MIT
/**
 * @file bsp_system.c
 * @brief GD32E507 CPU exclusion, static RAM bounds and build identity access.
 * @details Part of base. Entity: linker-owned regions and MCU interrupt mask.
 *          Prior: readable data/BSS only; no MMIO, heap or stack sampling.
 *          Time: bounded task-context queries; critical sections restore the prior mask.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#include "bsp_system.h"
#include "gd32e50x.h"
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
extern uint8_t _sdata[], _edata[], _sbss[], _ebss[];
extern const uint8_t __firmware_elf_sha256[];
uint32_t bsp_system_critical_enter(void)
{
    uint32_t state = __get_PRIMASK();
    __disable_irq();
    return state;
}
void bsp_system_critical_exit(uint32_t state)
{
    __set_PRIMASK(state);
}
void bsp_system_firmware_identity(uint8_t hash[32])
{
    memcpy(hash, __firmware_elf_sha256, 32u);
}
static bool contains(uintptr_t start,
                     uintptr_t end,
                     uint32_t address,
                     uint8_t size)
{
    return (uintptr_t)address >= start
        && (uintptr_t)address < end
        && (uintptr_t)size <= end - address;
}
const volatile void *bsp_system_sample_address(uint32_t address, uint8_t size)
{
    if (    size == 0u
         || address % (size > 4u ? 4u : size) != 0u)
        return NULL;

    if (    !contains((uintptr_t)_sdata, (uintptr_t)_edata, address, size)
         &&!contains((uintptr_t)_sbss, (uintptr_t)_ebss, address, size))
        return NULL;
    return (const volatile void *)(uintptr_t)address;
}
