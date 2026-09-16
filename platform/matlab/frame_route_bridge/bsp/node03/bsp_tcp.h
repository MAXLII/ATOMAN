// SPDX-License-Identifier: MIT
/**
 * @file bsp_tcp.h
 * @brief MATLAB node03 TCP endpoint configuration.
 * @details Static node configuration owned by this layer.
 * @author Max.Li
 * @date 2026-09-17
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License; see LICENSE in the project root.
 */
#ifndef BSP_TCP_H
#define BSP_TCP_H
#define BSP_TCP_FRAME_IP "0.0.0.0"
#define BSP_TCP_FRAME_PORT 5002u
#define BSP_TCP_PEER_IP "127.0.0.1"
#define BSP_TCP_PEER_PORT 5001u
#define BSP_TCP_DISCOVERY_NAME "MATLAB-SIM"
#define BSP_TCP_DISCOVERY_PORT 5000u

#include "sim_tcp.h"
void bsp_tcp_dbg_printf(const char *p_format, ...);
void bsp_tcp_dbg_tx(char *p_data, int len);
uint8_t bsp_tcp_dbg_rx_get_byte(uint8_t *p_data);
void bsp_tcp_iso_printf(const char *p_format, ...);
void bsp_tcp_iso_tx(char *p_data, int len);
uint8_t bsp_tcp_iso_rx_get_byte(uint8_t *p_data);
void sim_comm_stop(void);
#endif
