// SPDX-License-Identifier: MIT
/**
 * @file comm_link.c
 * @brief Connect CHB's PLECS TCP transport to FRAME protocol dispatch.
 * @details Protocol state stays within one registered communication context.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#include "bsp_tcp.h"
#include "comm.h"
#include "comm_addr.h"
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
