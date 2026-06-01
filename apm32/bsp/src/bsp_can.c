// SPDX-License-Identifier: MIT
/**
 * @file    bsp_can.c
 * @brief   APM32 CAN BSP compatibility module.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Provide no-op CAN hooks required by shared AC communication link registration
 *          - Keep CAN link traffic disabled until an APM32 CAN BSP is added
 *          - Preserve the same public interface shape as other platform BSPs
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-01
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
