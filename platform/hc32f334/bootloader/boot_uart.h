// SPDX-License-Identifier: MIT
/**
 * @file    boot_uart.h
 * @brief   Minimal HC32F334 bootloader USART2 interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Configure USART2 on PC10/PC4 at the FRAME baud rate
 *          - Provide byte polling receive and bounded transmit callbacks
 *          - Report shift-register idle state for IAP handoff sequencing
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Foreground polling only; no UART ISR ownership
 *          - Hardware access is confined to the HC32 bootloader platform
 *
 * @author  Max.Li
 * @date    2026-07-27
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef HC32F334_BOOT_UART_H
#define HC32F334_BOOT_UART_H

#include <stdint.h>

int32_t hc32_boot_uart_init(void);
uint8_t hc32_boot_uart_rx_get_byte(uint8_t *p_data);
void hc32_boot_uart_tx(char *p_data, int length);
void hc32_boot_uart_printf(const char *p_format, ...);
uint8_t hc32_boot_uart_tx_is_idle(void);

#endif /* HC32F334_BOOT_UART_H */
