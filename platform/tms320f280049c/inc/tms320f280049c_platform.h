// SPDX-License-Identifier: MIT
/**
 * @file    tms320f280049c_platform.h
 * @brief   TMS320F280049C demo platform interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Declare board initialization for the SECTION demo
 *          - Expose the 100 us scheduler tick
 *          - Provide LED and XDS110 virtual-COM byte-stream services
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The tick getter is safe for the 32-bit C28x CPU
 *          - GPIO and SCI access remain inside the platform layer
 *
 * @author  Max.Li
 * @date    2026-09-05
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef TMS320F280049C_PLATFORM_H
#define TMS320F280049C_PLATFORM_H

#include <stdint.h>

void tms320f280049c_device_init(void);
void tms320f280049c_platform_init(void);
uint32_t tms320f280049c_section_tick_get(void);
volatile uint32_t *tms320f280049c_section_tick_address_get(void);
void tms320f280049c_led_toggle(void);
uint8_t tms320f280049c_uart_rx_get_byte(uint8_t *p_data);
void tms320f280049c_uart_tx_write(const uint8_t *p_data, uint16_t length);

#endif /* TMS320F280049C_PLATFORM_H */
