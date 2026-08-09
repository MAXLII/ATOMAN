// SPDX-License-Identifier: MIT
/**
 * @file    bsp_enet.h
 * @brief   GD32E507Z-EVAL Ethernet BSP interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Expose RMII MAC and DP83848 initialization
 *          - Expose PHY link-state and receive-pending queries
 *          - Recognize FRAME Ethernet discovery requests and build device responses
 *          - Publish low-level initialization diagnostics
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The ENET path is polled from background context
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

#ifndef BSP_ENET_H
#define BSP_ENET_H

#include <stdint.h>

struct ip4_addr;
struct pbuf;
struct udp_pcb;

typedef enum
{
    BSP_ENET_DISCOVERY_NOT_HANDLED_E = 0, /* Datagram is unrelated and remains owned by the UDP service. */
    BSP_ENET_DISCOVERY_RESPONSE_SENT_E,   /* Discovery response was sent to the requesting host. */
    BSP_ENET_DISCOVERY_RESPONSE_ERROR_E   /* Discovery request matched but the response could not be sent. */
} bsp_enet_discovery_result_t;

extern volatile uint32_t g_bsp_enet_init_attempt_count;
extern volatile uint32_t g_bsp_enet_init_error_count;
extern volatile uint32_t g_bsp_enet_phy_read_error_count;

uint8_t bsp_enet_init(void);
uint8_t bsp_enet_link_is_up(void);
uint8_t bsp_enet_rx_frame_pending(void);

/**
 * @brief Recognize and answer one BSP-owned FRAME UDP discovery request.
 * @param[in] p_endpoint Bound UDP endpoint used to send the response.
 * @param[in] p_packet Received datagram retained by the caller for the complete call.
 * @param[in] p_remote_address Source IPv4 address that receives the response.
 * @param[in] remote_port Source UDP port that receives the response.
 * @return BSP discovery handling result; the caller always retains p_packet ownership.
 */
bsp_enet_discovery_result_t bsp_enet_discovery_udp_process(struct udp_pcb *p_endpoint,
                                                            const struct pbuf *p_packet,
                                                            const struct ip4_addr *p_remote_address,
                                                            uint16_t remote_port);

#endif /* BSP_ENET_H */
