// SPDX-License-Identifier: MIT
/**
 * @file    bsp_usart.h
 * @brief   GD32E507Z-EVAL USART BSP interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Expose USART0 debug transmit and receive operations
 *          - Provide the formatted-output callback required by Section links
 *          - Provide disabled ISO-link compatibility functions
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - USART0 transmit requests are serialized through a software ring buffer
 *          - Hardware access uses the GD32E50x standard peripheral library
 *
 * @author  Max.Li
 * @date    2026-08-08
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

extern volatile uint32_t g_bsp_usart_dbg_tx_drop_count;
extern volatile uint32_t g_bsp_usart_dbg_rx_drop_count;
extern volatile uint32_t g_bsp_usart_dbg_rx_error_count;

void bsp_usart_dbg_printf(const char *p_format, ...);
void bsp_usart_dbg_tx(char *p_data, int length);
int bsp_usart_dbg_tx_dma(const uint8_t *p_data, uint32_t length);
uint8_t bsp_usart_dbg_rx_get_byte(uint8_t *p_data);
void bsp_usart_dbg_irq_handler(void);

void bsp_usart_iso_printf(const char *p_format, ...);
void bsp_usart_iso_tx(char *p_data, int length);
uint8_t bsp_usart_iso_rx_get_byte(uint8_t *p_data);

#endif /* BSP_USART_H */
