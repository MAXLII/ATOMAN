// SPDX-License-Identifier: MIT
/**
 * @file bsp_tcp.c
 * @brief Project-owned endpoint registrations and TCP BSP adapters.
 * @details Static registration; serialized simulation callbacks; bounded storage.
 * @author Max.Li
 * @date 2026-09-17
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License; see LICENSE in the project root.
 */
#include "bsp_tcp.h"
#include "sim_discovery.h"

REG_SIM_TCP(p_dbg_tcp, "dbg", SIM_TCP_SERVER, "0.0.0.0", BSP_TCP_FRAME_PORT)

#if BSP_TCP_DISCOVERY_ENABLED
REG_SIM_DISCOVERY(local, BSP_TCP_DISCOVERY_NAME, BSP_TCP_NODE_ADDR, BSP_TCP_FRAME_PORT, BSP_TCP_DISCOVERY_PORT)
#if BSP_TCP_DISCOVERY_PEER_ADDR != 0
REG_SIM_DISCOVERY(peer, BSP_TCP_DISCOVERY_NAME, BSP_TCP_DISCOVERY_PEER_ADDR, BSP_TCP_DISCOVERY_PEER_PORT, BSP_TCP_DISCOVERY_PORT)
#endif
#endif

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
