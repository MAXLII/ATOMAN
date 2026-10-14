// SPDX-License-Identifier: MIT
/**
 * @file bsp_usart.h
 * @brief USART1 debug transport interface.
 * @details
 *          This file is part of the digital power framework project.
 *          Module responsibilities:
 *          - Expose bounded asynchronous TX and interrupt-fed RX on PA9/PA10
 *          Design notes:
 *          - C11 compatible; no dynamic allocation in platform drivers
 *          - Hardware access belongs to the BSP; framework bindings belong to src
 * @author Max.Li
 * @date 2026-10-14
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li.
 *          All rights reserved.
 *          This file is licensed under the MIT License.
 *          See the LICENSE file in the project root for full license text.
 */
#ifndef BSP_USART_H
#define BSP_USART_H
#include <stdint.h>
extern volatile uint32_t g_bsp_usart_dbg_tx_drop_count;  /* Rejected whole TX messages. */
extern volatile uint32_t g_bsp_usart_dbg_rx_drop_count;  /* Bytes lost to RX queue overflow. */
extern volatile uint32_t g_bsp_usart_dbg_rx_error_count; /* USART framing/noise/overrun events. */
void bsp_usart_init(void);
void bsp_usart_dbg_irq_handler(void);
void bsp_usart_dbg_tx_irq_handler(void);
/** @param p_data Destination byte. @return 1 when a byte was consumed, else 0. */
uint8_t bsp_usart_dbg_rx_get_byte(uint8_t *p_data);
/** @param p_data Message bytes. @param length Positive byte count, at most 1023. */
void bsp_usart_dbg_tx(char *p_data, int length);
/** @param p_format printf-style bounded debug message format. */
void bsp_usart_dbg_printf(const char *p_format, ...);
#endif /* BSP_USART_H */
