// SPDX-License-Identifier: MIT
/**
 * @file bsp_tcp.h
 * @brief Build-time configuration for simulation TCP and discovery endpoints.
 * @details Node targets override defaults; discovery names follow the host platform.
 * @author Max.Li
 * @date 2026-09-16
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License; see LICENSE in the project root.
 */
#ifndef BSP_TCP_H
#define BSP_TCP_H

#include "platform.h"

#ifndef BSP_TCP_NODE_ADDR
#define BSP_TCP_NODE_ADDR 2u
#endif
#ifndef BSP_TCP_FRAME_PORT
#define BSP_TCP_FRAME_PORT 5000u
#endif
#ifndef BSP_TCP_PEER_ROLE
#define BSP_TCP_PEER_ROLE 0 /* 0 disabled, 1 client, 2 server. */
#endif
#ifndef BSP_TCP_PEER_PORT
#define BSP_TCP_PEER_PORT 5001u
#endif
#ifndef BSP_TCP_DISCOVERY_PORT
#define BSP_TCP_DISCOVERY_PORT 5000u
#endif
#ifndef BSP_TCP_DISCOVERY_ENABLED
#define BSP_TCP_DISCOVERY_ENABLED 1
#endif
#ifndef BSP_TCP_DISCOVERY_PEER_ADDR
#define BSP_TCP_DISCOVERY_PEER_ADDR 0u
#endif
#ifndef BSP_TCP_DISCOVERY_PEER_PORT
#define BSP_TCP_DISCOVERY_PEER_PORT 0u
#endif
#ifndef BSP_TCP_DISCOVERY_NAME
#define BSP_TCP_DISCOVERY_NAME "PLECS-SIM"
#endif
#if BSP_TCP_PEER_ROLE < 0 || BSP_TCP_PEER_ROLE > 2
#error "BSP_TCP_PEER_ROLE must be 0, 1 or 2."
#endif

#include "sim_tcp.h"
void bsp_tcp_dbg_printf(const char *p_format, ...);
void bsp_tcp_dbg_tx(char *p_data, int len);
uint8_t bsp_tcp_dbg_rx_get_byte(uint8_t *p_data);
void sim_comm_stop(void);
#endif
