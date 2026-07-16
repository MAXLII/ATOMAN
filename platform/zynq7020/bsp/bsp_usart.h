// SPDX-License-Identifier: MIT
/**
 * @file    bsp_usart.h
 * @brief   Zynq-7020 PS UART1 communication interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Initialize PS UART1 on MIO48 and MIO49 at 115200 baud
 *          - Provide byte polling for section Shell and comm protocol parsing
 *          - Provide blocking transmit and bounded formatted output
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The first platform version uses polling instead of DMA
 *          - Hardware access is isolated in the Zynq BSP
 *
 * @author  Max.Li
 * @date    2026-07-17
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef BSP_USART_H
#define BSP_USART_H

#include <stdint.h>

int32_t bsp_usart_init(void);
void bsp_usart_dbg_printf(const char *format, ...);
void bsp_usart_dbg_tx(char *data, int length);
uint8_t bsp_usart_dbg_rx_get_byte(uint8_t *data);

#endif /* BSP_USART_H */
