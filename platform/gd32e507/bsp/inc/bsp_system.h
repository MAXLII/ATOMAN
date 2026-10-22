// SPDX-License-Identifier: MIT
/**
 * @file bsp_system.h
 * @brief MCU critical section, linked RAM validation and firmware identity.
 * @details Part of base. No protocol or sampler ownership; task callers restore IRQ state.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#ifndef BSP_SYSTEM_H
#define BSP_SYSTEM_H
#include <stdint.h>
uint32_t bsp_system_critical_enter(void);
void bsp_system_critical_exit(uint32_t state);
void bsp_system_firmware_identity(uint8_t hash[32]);
const volatile void *bsp_system_sample_address(uint32_t address, uint8_t size);
#endif
