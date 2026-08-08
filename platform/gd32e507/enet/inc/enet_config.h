// SPDX-License-Identifier: MIT
/**
 * @file    enet_config.h
 * @brief   GD32E507 Ethernet communication configuration.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Define the board MAC and static IPv4 address
 *          - Define the TCP FRAME protocol and UDP echo service ports
 *          - Provide one configuration source for the LwIP adapter and application
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation outside the LwIP fixed heap
 *          - The address is selected for the directly connected 192.168.1.0/24 network
 *          - Hardware access is abstracted through the GD32E507 ENET BSP
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

#ifndef ENET_CONFIG_H
#define ENET_CONFIG_H

#define BOARD_MAC_ADDR0 0x02u
#define BOARD_MAC_ADDR1 0x00u
#define BOARD_MAC_ADDR2 0x00u
#define BOARD_MAC_ADDR3 0xE5u
#define BOARD_MAC_ADDR4 0x07u
#define BOARD_MAC_ADDR5 0x01u

#define ENET_CONFIG_IP_ADDR0 192u
#define ENET_CONFIG_IP_ADDR1 168u
#define ENET_CONFIG_IP_ADDR2 1u
#define ENET_CONFIG_IP_ADDR3 101u

#define ENET_CONFIG_NETMASK_ADDR0 255u
#define ENET_CONFIG_NETMASK_ADDR1 255u
#define ENET_CONFIG_NETMASK_ADDR2 255u
#define ENET_CONFIG_NETMASK_ADDR3 0u

#define ENET_CONFIG_GATEWAY_ADDR0 0u
#define ENET_CONFIG_GATEWAY_ADDR1 0u
#define ENET_CONFIG_GATEWAY_ADDR2 0u
#define ENET_CONFIG_GATEWAY_ADDR3 0u

#define ENET_CONFIG_TCP_FRAME_PORT 5000u
#define ENET_CONFIG_UDP_ECHO_PORT 5000u

#endif /* ENET_CONFIG_H */
