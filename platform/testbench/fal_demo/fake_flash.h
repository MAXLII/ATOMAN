// SPDX-License-Identifier: MIT
/**
 * @file    fake_flash.h
 * @brief   Expose deterministic Flash fault injection for storage service tests.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Expose deterministic Flash fault injection for storage service tests.
 *          - Keep operations bounded and report invalid requests.
 *          Design notes:
 *          - C11 compatible; no dynamic memory allocation.
 *          - Task-context API; not ISR-safe.
 *          - Hardware access is confined to the platform BSP.
 * @author  Max.Li
 * @date    2026-09-05
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef FAKE_FLASH_H
#define FAKE_FLASH_H
#include "bsp_flash.h"
void fake_reset(void);
void fake_present(uint8_t device, uint8_t present);
void fake_read_fault(uint8_t device, uint32_t address);
void fake_program_fault(uint8_t device, uint32_t address);
void fake_flip(uint8_t device, uint32_t address);
uint32_t fake_writes(void);
uint32_t fake_prefix_writes(void);
#endif
