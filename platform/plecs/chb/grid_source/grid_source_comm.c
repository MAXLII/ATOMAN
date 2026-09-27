// SPDX-License-Identifier: MIT
/**
 * @file grid_source_comm.c
 * @brief Independent grid-source FRAME TCP endpoint and protocol binding.
 * @details Lifecycle and task scheduling are provided by the shared PLECS entry.
 * @author Max.Li
 * @date 2026-09-27
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. Licensed under the MIT License.
 */
#include "plecs.h"
#include "section.h"
#include "sim_tcp.h"
#include "sim_discovery.h"
#include "comm.h"
#include "comm_addr.h"
#include "comm_route_cfg.h"
#include "my_math.h"
#include <stdarg.h>

#define GRID_SOURCE_TCP_PORT 5005u /* Separate endpoint from the CHB controller. */

REG_SIM_TCP(p_grid_tcp, "grid", SIM_TCP_SERVER, "0.0.0.0", GRID_SOURCE_TCP_PORT)
REG_SIM_TCP(p_peer_tcp, "peer", SIM_TCP_CLIENT, "127.0.0.1", CHB_ROUTE_PEER_TCP_PORT)
REG_SIM_DISCOVERY(grid_source, "PLECS-GRID", HOST_ADDR, GRID_SOURCE_TCP_PORT, 5000u)
DECLARE_COMM_CTX(grid_ctx, COMM_MAX_PAYLOAD_SIZE, HOST_ADDR, 1, "grid");

/** @param p_data Outgoing bytes. @param len Byte count. */
static void grid_tx(char *p_data, int len)
{
    sim_tcp_tx(p_grid_tcp, p_data, len);
}
/** @param p_data Next incoming byte. @return Nonzero when a byte was obtained. */
static uint8_t grid_rx(uint8_t *p_data)
{
    return sim_tcp_rx_get_byte(p_grid_tcp, p_data);
}
/** @param p_format Diagnostic format string. */
static void grid_printf(const char *p_format, ...)
{
    va_list args; /* Forwarded formatting arguments. */
    va_start(args, p_format);
    sim_tcp_vprintf(p_grid_tcp, p_format, args);
    va_end(args);
}
static section_link_tx_func_t grid_link_tx = { /* Protocol transport callbacks. */
    .my_printf = grid_printf,
    .tx_by_dma = grid_tx,
};
static const section_link_handler_item_t grid_handlers[] = { /* One FRAME protocol dispatcher. */
    {.func = comm_run, .ctx = &grid_ctx},
};
REG_LINK(1, grid_link_tx, grid_rx, grid_handlers, ARRAY_SIZE(grid_handlers))

DECLARE_COMM_CTX(peer_ctx, COMM_MAX_PAYLOAD_SIZE, HOST_ADDR, 2, "peer");

/** @param p_data Outgoing bytes. @param len Byte count. */
static void peer_tx(char *p_data, int len)
{
    sim_tcp_tx(p_peer_tcp, p_data, len);
}

/** @param p_data Next incoming byte. @return Nonzero when a byte was obtained. */
static uint8_t peer_rx(uint8_t *p_data)
{
    return sim_tcp_rx_get_byte(p_peer_tcp, p_data);
}

/** @param p_format Format string sent to the controller peer. */
static void peer_printf(const char *p_format, ...)
{
    va_list args; /* Arguments forwarded to the internal transport. */
    va_start(args, p_format);
    sim_tcp_vprintf(p_peer_tcp, p_format, args);
    va_end(args);
}

static section_link_tx_func_t peer_link_tx = {
    .my_printf = peer_printf,
    .tx_by_dma = peer_tx,
}; /**< Internal transport, independent of FRAME's TCP connection. */
static const section_link_handler_item_t peer_handlers[] = {
    {.func = comm_run, .ctx = &peer_ctx},
}; /**< Peer requests and routed responses use their own parser. */
REG_LINK(2, peer_link_tx, peer_rx, peer_handlers, ARRAY_SIZE(peer_handlers))

REG_COMM_ROUTE(1, 2, CHB_ROUTE_CTRL_ADDR)
REG_COMM_ROUTE(2, 1, CHB_ROUTE_FRAME_ADDR)

void sim_comm_stop(void)
{
    sim_discovery_stop();
    sim_tcp_stop();
}
