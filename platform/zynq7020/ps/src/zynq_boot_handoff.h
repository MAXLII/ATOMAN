// SPDX-License-Identifier: MIT
/**
 * @file    zynq_boot_handoff.h
 * @brief   Shared retained handoff between Zynq IAP and Bootloader images.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Inspect the retained request written by the independent IAP service
 *          - Clear a consumed request before launching the IAP
 *          - Keep Bootloader-side retained state handling platform-local
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The record occupies a fixed on-chip-memory location
 *          - Cache and interrupt handling remain confined to this platform module
 *
 * @author  Max.Li
 * @date    2026-07-27
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef ZYNQ_BOOT_HANDOFF_H
#define ZYNQ_BOOT_HANDOFF_H

#include "bootloader_core.h"

bootloader_boot_reason_t zynq_boot_request_get(void);
bootloader_result_t zynq_boot_request_clear(void);

#endif /* ZYNQ_BOOT_HANDOFF_H */
