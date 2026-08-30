// SPDX-License-Identifier: MIT
/**
 * @file    peer_tcp_link.h
 * @brief   PLECS node-to-node TCP link interface.
 * @details
 *          This file is part of the base PLECS common platform.
 *
 *          Module responsibilities:
 *          - Define the internal TCP endpoint and Section link identity
 *          - Select client or server behavior at build time
 *          - Expose transport lifecycle and connection-state queries
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The internal endpoint is restricted to the Windows loopback interface
 *          - Hardware access is not used by this simulation module
 *
 * @author  Max.Li
 * @date    2026-08-30
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef PEER_TCP_LINK_H
#define PEER_TCP_LINK_H

#include <stdint.h>

#define PEER_TCP_LINK_PORT (5001u)
#define PEER_TCP_LINK_ID (2u)

#define PEER_TCP_ROLE_CLIENT (1)
#define PEER_TCP_ROLE_SERVER (2)

void peer_tcp_link_start(void);
void peer_tcp_link_stop(void);
uint8_t peer_tcp_link_is_connected(void);

#endif /* PEER_TCP_LINK_H */
