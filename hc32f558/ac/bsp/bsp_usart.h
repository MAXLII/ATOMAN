// SPDX-License-Identifier: MIT
/**
 * @file    bsp_usart.h
 * @brief   HC32F558 USART BSP interface.
 * @details
 *          This file is part of the HC32F558 project.
 *
 *          Module responsibilities:
 *          - Declare the debug USART initialization interface
 *          - Declare DMA ring-buffer TX and RX helpers
 *          - Declare a fixed-buffer printf helper for debug output
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented by callers
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-06
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef __BSP_USART_H__
#define __BSP_USART_H__

#include "hc32_ll.h"

#ifdef __cplusplus
extern "C" {
#endif

void bsp_usart_init(void);
void bsp_usart_dbg_tx(const char *ptr, int len);
uint32_t bsp_usart_dbg_tx_dma(const uint8_t *data, uint32_t len);
void bsp_usart_dbg_printf(const char *format, ...);
uint8_t bsp_usart_dbg_rx_get_byte(uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif
