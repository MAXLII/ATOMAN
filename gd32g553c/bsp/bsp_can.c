// SPDX-License-Identifier: MIT
/**
 * @file    bsp_can.c
 * @brief   GD32 CAN BSP compatibility module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Provide no-op CAN hooks required by the shared AC comm_link registration
 *          - Keep CAN link traffic disabled on GD32 until a real CAN BSP is added
 *          - Preserve the same public interface shape as the HC32 CAN BSP
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-10
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "bsp_can.h"

#include <stdarg.h>

void bsp_can_dbg_printf(const char *__format, ...)
{
    va_list args;

    (void)__format;
    va_start(args, __format);
    va_end(args);
}

void bsp_can_dbg_tx(char *ptr, int len)
{
    (void)ptr;
    (void)len;
}

uint8_t bsp_can_dbg_rx_get_byte(uint8_t *p_data)
{
    (void)p_data;
    return 0U;
}
