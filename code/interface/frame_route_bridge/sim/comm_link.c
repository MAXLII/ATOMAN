// SPDX-License-Identifier: MIT
/**
 * @file comm_link.c
 * @brief Project-owned SECTION link registration for the simulation BSP.
 * @details Binds BSP functions and protocol contexts through registration tables.
 *          The protocol component owns connection resets and parser timing.
 * @author Max.Li
 * @date 2026-09-16
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the project root.
 */
#include "bsp_tcp.h"
#include "comm.h"
#include "comm_addr.h"
#include "section.h"
#include "my_math.h"

DECLARE_COMM_CTX(dbg_ctx, COMM_MAX_PAYLOAD_SIZE, HOST_ADDR, 1, "dbg");
static section_link_tx_func_t dbg_tx = {
    .my_printf = bsp_tcp_dbg_printf,
    .tx_by_dma = bsp_tcp_dbg_tx,
}; /**< Byte output supplied by the shared simulation BSP. */
static const section_link_handler_item_t dbg_handlers[] = {
    {.func = comm_run, .ctx = &dbg_ctx},
}; /**< Protocol dispatch stays in the project interface. */
REG_LINK(1, dbg_tx, bsp_tcp_dbg_rx_get_byte, dbg_handlers, ARRAY_SIZE(dbg_handlers))

DECLARE_COMM_CTX(iso_ctx, COMM_MAX_PAYLOAD_SIZE, HOST_ADDR, 2, "iso");
static section_link_tx_func_t iso_tx = {
    .my_printf = bsp_tcp_iso_printf,
    .tx_by_dma = bsp_tcp_iso_tx,
}; /**< Byte output supplied by the shared simulation BSP. */
static const section_link_handler_item_t iso_handlers[] = {
    {.func = comm_run, .ctx = &iso_ctx},
}; /**< Protocol dispatch stays in the project interface. */
REG_LINK(2, iso_tx, bsp_tcp_iso_rx_get_byte, iso_handlers, ARRAY_SIZE(iso_handlers))
