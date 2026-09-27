// SPDX-License-Identifier: MIT
/**
 * @file bsp_tcp.c
 * @brief Register and adapt the CHB FRAME TCP server.
 * @details The shared transport owns endpoint lifecycle and bounded buffers.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#include "bsp_tcp.h"
#include "sim_discovery.h"
#include "comm_route_cfg.h"

REG_SIM_TCP(p_dbg_tcp, "dbg", SIM_TCP_SERVER, "0.0.0.0", CHB_FRAME_TCP_PORT)
REG_SIM_TCP(p_peer_tcp, "peer", SIM_TCP_SERVER, "127.0.0.1", CHB_ROUTE_PEER_TCP_PORT)
REG_SIM_DISCOVERY(chb, "PLECS-CHB", CHB_FRAME_NODE_ADDR, CHB_FRAME_TCP_PORT, 5000u)

void bsp_tcp_dbg_tx(char *p_data, int len)
{
    sim_tcp_tx(p_dbg_tcp, p_data, len);
}

uint8_t bsp_tcp_dbg_rx_get_byte(uint8_t *p_data)
{
    return sim_tcp_rx_get_byte(p_dbg_tcp, p_data);
}

void bsp_tcp_dbg_printf(const char *p_format, ...)
{
    va_list args;
    va_start(args, p_format);
    sim_tcp_vprintf(p_dbg_tcp, p_format, args);
    va_end(args);
}

void sim_comm_stop(void)
{
    sim_discovery_stop();
    sim_tcp_stop();
}

void bsp_tcp_peer_tx(char *p_data, int len)
{
    sim_tcp_tx(p_peer_tcp, p_data, len);
}

uint8_t bsp_tcp_peer_rx_get_byte(uint8_t *p_data)
{
    return sim_tcp_rx_get_byte(p_peer_tcp, p_data);
}

void bsp_tcp_peer_printf(const char *p_format, ...)
{
    va_list args; /* Arguments forwarded to the internal transport. */
    va_start(args, p_format);
    sim_tcp_vprintf(p_peer_tcp, p_format, args);
    va_end(args);
}
