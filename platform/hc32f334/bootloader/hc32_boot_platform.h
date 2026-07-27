// SPDX-License-Identifier: MIT
/**
 * @file    hc32_boot_platform.h
 * @brief   HC32F334 retained boot reason and IAP jump interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Publish Bootloader Core platform callbacks for Cortex-M startup validation
 *          - Store and clear an IAP upgrade request across software reset
 *          - Transfer control between the internal bootloader and IAP images
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Jump executes with interrupts disabled
 *          - HC32 registers and memory ranges remain in this platform module
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

#ifndef HC32F334_BOOT_PLATFORM_H
#define HC32F334_BOOT_PLATFORM_H

#include "bootloader_core.h"

bootloader_platform_ops_t hc32_boot_platform_ops_make(void);

#endif /* HC32F334_BOOT_PLATFORM_H */
