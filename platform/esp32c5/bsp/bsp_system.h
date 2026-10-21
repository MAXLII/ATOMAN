// SPDX-License-Identifier: MIT
/**
 * @file bsp_system.h
 * @brief Board clock and reset primitives without framework dependencies.
 * @details Part of base. SDK monotonic time is truncated only after unit conversion.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#ifndef BSP_SYSTEM_H
#define BSP_SYSTEM_H
#include <stdint.h>
/** Single-core task/ISR exclusion; exit restores the previous enabled interrupt mask. */
uint32_t bsp_system_critical_enter(void);
void bsp_system_critical_exit(uint32_t state);
/** SHA-256 identity from the SDK application descriptor. Task context only. */
void bsp_system_firmware_identity(uint8_t hash[32]);
/** Validate linked static RAM and natural alignment; never admit arbitrary MMIO. */
const volatile void *bsp_system_sample_address(uint32_t address, uint8_t size);
/** @return Low 32 bits of the monotonic 100 us clock. */
uint32_t bsp_system_tick_100us_get(void);
/** @return Low 32 bits of the monotonic microsecond clock. */
uint32_t bsp_system_time_us_get(void);
/** @brief Restart the MCU through the SDK. */
void bsp_system_reset(void);
#endif /* BSP_SYSTEM_H */
