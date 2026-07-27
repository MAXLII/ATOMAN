// SPDX-License-Identifier: MIT
/**
 * @file    boot_interrupt.c
 * @brief   Minimal HC32F334 bootloader interrupt implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Advance the platform time base from the SysTick exception
 *          - Leave unused exceptions on the startup file default handler
 *          - Keep application scheduler and fault-capture code out of the boot image
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - SysTick is the only interrupt used by the polling bootloader
 *          - Hardware access remains in the platform layer
 *
 * @author  Max.Li
 * @date    2026-07-28
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "systick.h"

void SysTick_Handler(void);

/** Advance the 100 us platform time base. */
void SysTick_Handler(void)
{
    delay_decrement();
}
