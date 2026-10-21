// SPDX-License-Identifier: MIT
/**
 * @file bsp_usart.h
 * @brief Type-C USB Serial/JTAG byte transport, using the base BSP naming.
 * @details Part of base. One task owns init/read/write. RX is nonblocking;
 *          TX waits at most 20 ms and reports partial acceptance. No hardware UART.
 * @author Max.Li
 * @date 2026-10-21
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#ifndef BSP_USART_H
#define BSP_USART_H
#include <stdint.h>
#define BSP_USART_TRANSFER_MAX 512u /**< Maximum work per read/write call. */
/** @return ESP_OK or ESP-IDF error. Initialize once from the main task/core. */
int32_t bsp_usart_init(void);
/** @param p_data Receive destination. @param capacity Capacity, 1..512 bytes.
 * @return Bytes received, 0 when empty, -1 for invalid arguments/uninitialized driver.
 */
int32_t bsp_usart_read(uint8_t *p_data, uint32_t capacity);
/** @param p_data Source bytes. @param length Byte count, 1..512.
 * @return Bytes accepted, 0 on backpressure, -1 for invalid arguments/uninitialized driver.
 * @details Caller retains and retries any unsent tail. No implicit DMA ownership.
 */
int32_t bsp_usart_write(const uint8_t *p_data, uint32_t length);
/** @param p_data Byte destination. @return 1 when a byte was received, otherwise 0. */
uint8_t bsp_usart_dbg_rx_get_byte(uint8_t *p_data);
#endif /* BSP_USART_H */
