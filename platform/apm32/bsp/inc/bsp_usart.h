// SPDX-License-Identifier: MIT
/**
 * @file    bsp_usart.h
 * @brief   APM32 USART BSP interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Expose debug and isolated USART TX/RX hooks for shared link code
 *          - Provide printf and DMA TX-compatible entry points
 *          - Hide APM32 USART and DMA channel details behind BSP APIs
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - RX uses circular DMA buffers; TX uses software ring buffers and DMA
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
#ifndef __BSP_USART_H
#define __BSP_USART_H

#include <stdint.h>

void bsp_usart_dbg_printf(const char *__format, ...);
void bsp_usart_dbg_tx(char *ptr, int len);
int bsp_usart_dbg_tx_dma(const uint8_t *p_data, uint32_t len);
uint8_t bsp_usart_dbg_rx_get_byte(uint8_t *p_data);

void bsp_usart_iso_printf(const char *__format, ...);
void bsp_usart_iso_tx(char *ptr, int len);
uint8_t bsp_usart_iso_rx_get_byte(uint8_t *p_data);

#endif
