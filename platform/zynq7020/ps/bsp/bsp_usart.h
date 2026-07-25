// SPDX-License-Identifier: MIT
/**
 * @file    bsp_usart.h
 * @brief   Zynq-7020 PS UART1 and PL UART DMA communication interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Initialize PS UART1 on MIO48 and MIO49 at 921600 baud
 *          - Configure the PL UART and its autonomous DDR ring DMA
 *          - Hide PL ring counters, DDR addresses, barriers, and recovery
 *          - Provide independent byte streams for COM6 and COM7
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - PS UART1 uses polling and PL UART traffic uses DDR ring DMA
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

typedef enum
{
    BSP_USART_PL_PARITY_NONE = 0,
    BSP_USART_PL_PARITY_EVEN = 1,
    BSP_USART_PL_PARITY_ODD = 2,
} bsp_usart_pl_parity_t;

typedef struct
{
    uint32_t baud_rate;
    uint8_t data_bits;
    bsp_usart_pl_parity_t parity;
    uint8_t stop_bits;
    uint8_t internal_loopback;
} bsp_usart_pl_config_t;

typedef struct
{
    uint32_t version;
    uint32_t status;
    uint32_t irq_status;
    uint32_t uart_errors;
    uint32_t dma_errors;
    uint32_t dma_stop_reason;
    uint32_t rx_produced;
    uint32_t rx_consumed;
    uint32_t tx_produced;
    uint32_t tx_consumed;
    uint32_t error_irq_count;
    uint32_t error_irq_latched;
} bsp_usart_pl_status_t;

int32_t bsp_usart_init(void);
void bsp_usart_dbg_printf(const char *format, ...);
void bsp_usart_dbg_tx(char *data, int length);
uint8_t bsp_usart_dbg_rx_get_byte(uint8_t *data);

int32_t bsp_usart_pl_init(void);
int32_t bsp_usart_pl_configure(const bsp_usart_pl_config_t *config);
int32_t bsp_usart_pl_tx_dma(const uint8_t *data, uint32_t length);
void bsp_usart_pl_printf(const char *format, ...);
uint8_t bsp_usart_pl_rx_get_byte(uint8_t *data);
void bsp_usart_pl_status_get(bsp_usart_pl_status_t *status);
int32_t bsp_usart_pl_reset(void);
void bsp_usart_pl_error_clear(void);

#endif /* BSP_USART_H */
