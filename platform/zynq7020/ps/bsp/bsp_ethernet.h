// SPDX-License-Identifier: MIT
/**
 * @file    bsp_ethernet.h
 * @brief   Zynq-7020 PS GEM0 and lwIP TCP service interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Expose initialization and polling for the PS GEM0 Ethernet interface
 *          - Provide byte-stream receive and transmit operations to Section links
 *          - Report the active TCP client and bounded receive-buffer state
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation in project-owned code
 *          - GEM DMA interrupt registration is provided by the Xilinx lwIP adapter
 *          - Hardware access is abstracted through the Xilinx XEmacPs BSP
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

#ifndef BSP_ETHERNET_H
#define BSP_ETHERNET_H

#include <stdint.h>

typedef struct
{
    uint8_t initialized;
    uint8_t client_connected;
    uint8_t receive_overflow;
    uint32_t received_bytes;
    uint32_t transmitted_bytes;
    uint32_t receive_buffer_used;
} bsp_ethernet_status_t;

int32_t bsp_ethernet_init(void);
void bsp_ethernet_poll(void);
void bsp_ethernet_prepare_handoff(void);
uint8_t bsp_ethernet_rx_get_byte(uint8_t *p_data);
int32_t bsp_ethernet_tx(const uint8_t *p_data, uint32_t length);
void bsp_ethernet_printf(const char *format, ...);
void bsp_ethernet_status_get(bsp_ethernet_status_t *p_status);

#endif /* BSP_ETHERNET_H */
