// SPDX-License-Identifier: MIT
/**
 * @file    bsp_can.h
 * @brief   GD32 CAN BSP compatibility interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Provide the CAN function names required by the shared comm_link module
 *          - Keep GD32 builds linkable when CAN hardware support is not enabled yet
 *          - Define a replacement point for a future GD32 CAN driver implementation
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
#ifndef __BSP_CAN_H__
#define __BSP_CAN_H__

#include <stdint.h>

void bsp_can_dbg_printf(const char *__format, ...);
void bsp_can_dbg_tx(char *ptr, int len);
uint8_t bsp_can_dbg_rx_get_byte(uint8_t *p_data);

#endif
