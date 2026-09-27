// SPDX-License-Identifier: MIT
/**
 * @file comm_link.c
 * @brief Connect CHB's PLECS TCP transport to FRAME protocol dispatch.
 * @details FRAME and the grid peer have separate protocol contexts.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#include "bsp_tcp.h"
#include "comm.h"
#include "comm_addr.h"
#include "comm_route_cfg.h"
#include "my_math.h"
#include "section.h"

DECLARE_COMM_CTX(dbg_ctx, COMM_MAX_PAYLOAD_SIZE, HOST_ADDR, 1, "dbg");
static section_link_tx_func_t dbg_tx = {
    .my_printf = bsp_tcp_dbg_printf,
    .tx_by_dma = bsp_tcp_dbg_tx,
};
static const section_link_handler_item_t dbg_handlers[] = {
    {.func = comm_run, .ctx = &dbg_ctx},
};
REG_LINK(1, dbg_tx, bsp_tcp_dbg_rx_get_byte, dbg_handlers, ARRAY_SIZE(dbg_handlers))

DECLARE_COMM_CTX(peer_ctx, COMM_MAX_PAYLOAD_SIZE, HOST_ADDR, 2, "peer");
static section_link_tx_func_t peer_tx = {
    .my_printf = bsp_tcp_peer_printf,
    .tx_by_dma = bsp_tcp_peer_tx,
}; /**< Internal transport, independent of FRAME's TCP connection. */
static const section_link_handler_item_t peer_handlers[] = {
    {.func = comm_run, .ctx = &peer_ctx},
}; /**< Peer requests and routed responses use their own parser. */
REG_LINK(2, peer_tx, bsp_tcp_peer_rx_get_byte, peer_handlers, ARRAY_SIZE(peer_handlers))

REG_COMM_ROUTE(1, 2, CHB_ROUTE_GRID_ADDR)
REG_COMM_ROUTE(2, 1, CHB_ROUTE_FRAME_ADDR)
