// SPDX-License-Identifier: MIT
/**
 * @file comm_link.c
 * @brief Project-owned SECTION link registration for the simulation BSP.
 * @details Binds BSP functions and protocol contexts through registration tables.
 *          The protocol component owns connection resets and parser timing.
 * @author Max.Li
 * @date 2026-09-16
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License; see LICENSE in the project root.
 */
#include "bsp_tcp.h"
#include "comm.h"
#include "comm_link.h"
#include "sim_discovery.h"
#include "section.h"
#include "my_math.h"

DECLARE_COMM_CTX(dbg_ctx, COMM_MAX_PAYLOAD_SIZE, COMM_LINK_DEVICE_ADDR, TCP_DBG_LINK, "dbg");
static section_link_tx_func_t dbg_tx = {
    .my_printf = bsp_tcp_dbg_printf,
    .tx_by_dma = bsp_tcp_dbg_tx,
}; /**< Byte output supplied by the shared simulation BSP. */
static const section_link_handler_item_t dbg_handlers[] = {
    {.func = comm_run, .ctx = &dbg_ctx},
}; /**< Protocol dispatch stays in the project interface. */
REG_LINK(TCP_DBG_LINK, dbg_tx, bsp_tcp_dbg_rx_get_byte, dbg_handlers, ARRAY_SIZE(dbg_handlers))

DECLARE_COMM_CTX(iso_ctx, COMM_MAX_PAYLOAD_SIZE, COMM_LINK_DEVICE_ADDR, TCP_ISO_LINK, "iso");
static section_link_tx_func_t iso_tx = {
    .my_printf = bsp_tcp_iso_printf,
    .tx_by_dma = bsp_tcp_iso_tx,
}; /**< Byte output supplied by the shared simulation BSP. */
static const section_link_handler_item_t iso_handlers[] = {
    {.func = comm_run, .ctx = &iso_ctx},
}; /**< Protocol dispatch stays in the project interface. */
REG_LINK(TCP_ISO_LINK, iso_tx, bsp_tcp_iso_rx_get_byte, iso_handlers, ARRAY_SIZE(iso_handlers))


REG_COMM_ROUTE(TCP_DBG_LINK, TCP_ISO_LINK, COMM_LINK_PEER_ADDR)
REG_COMM_ROUTE(TCP_ISO_LINK, TCP_DBG_LINK, COMM_LINK_PC_ADDR)

REG_SIM_DISCOVERY(local, BSP_TCP_DISCOVERY_NAME, COMM_LINK_DEVICE_ADDR,
                  BSP_TCP_FRAME_PORT, BSP_TCP_DISCOVERY_PORT)
REG_SIM_DISCOVERY(peer, BSP_TCP_DISCOVERY_NAME, COMM_LINK_PEER_ADDR,
                  BSP_TCP_DISCOVERY_PEER_PORT, BSP_TCP_DISCOVERY_PORT)
