// SPDX-License-Identifier: MIT
/**
 * @file    route_bridge_app.c
 * @brief   PLECS FRAME route-bridge node application.
 * @details
 *          This file is part of the base PLECS FRAME route-bridge project.
 *
 *          Module responsibilities:
 *          - Register node identity, writable value, counters, and link status as FRAME parameters
 *          - Echo command 0x30/0x01 with a direct ACK from the addressed node
 *          - Register transparent routes between Frame, node 0x02, and node 0x03
 *          - Publish node value and internal-link state to the PLECS model
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The shared dispatch lock serializes transport and simulation callbacks
 *          - Hardware access is not used by this simulation project
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
#include "route_bridge_app.h"

#include "comm.h"
#include "frame_tcp_server.h"
#include "peer_tcp_link.h"
#include "plecs.h"
#include "section.h"
#include "shell.h"

#include <stddef.h>
#include <stdint.h>

#ifndef PLECS_NODE_ADDR
#error "PLECS_NODE_ADDR must be 0x02 or 0x03"
#endif

#if (PLECS_NODE_ADDR != 0x02) && (PLECS_NODE_ADDR != 0x03)
#error "PLECS_NODE_ADDR has an unsupported value"
#endif

typedef struct
{
    uint32_t node_addr; /**< Fixed protocol address of this loaded DLL. */
    uint32_t node_value; /**< Value written through FRAME and shown in the PLECS model. */
    uint32_t loopback_count; /**< Valid loopback requests handled by this node. */
    uint32_t peer_connected; /**< Normalized internal TCP connection state. */
    uint32_t frame_connected; /**< Normalized FRAME TCP connection state for this node. */
} route_bridge_state_t;

static route_bridge_state_t route_state = {
    .node_addr = (uint32_t)PLECS_NODE_ADDR,
    .node_value = (uint32_t)PLECS_NODE_ADDR,
    .loopback_count = 0u,
    .peer_connected = 0u,
    .frame_connected = 0u,
}; /**< Mutable state owned by one independently loaded node DLL. */

REG_SHELL_VAR(NODE_ADDR, route_state.node_addr, SHELL_UINT32, 0u, 0u, NULL, SHELL_STA_READ_ONLY)
REG_SHELL_VAR(NODE_VALUE, route_state.node_value, SHELL_UINT32, 1000000u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(LOOPBACK_COUNT,
              route_state.loopback_count,
              SHELL_UINT32,
              0u,
              0u,
              NULL,
              SHELL_STA_READ_ONLY)
REG_SHELL_VAR(PEER_CONNECTED,
              route_state.peer_connected,
              SHELL_UINT32,
              0u,
              0u,
              NULL,
              SHELL_STA_READ_ONLY)

REG_SHELL_VAR(FRAME_CONNECTED,
              route_state.frame_connected,
              SHELL_UINT32,
              0u,
              0u,
              NULL,
              SHELL_STA_READ_ONLY)

#if (PLECS_NODE_ADDR == 0x02)
REG_COMM_ROUTE(1, 2, 0x03)
REG_COMM_ROUTE(2, 1, 0x01)
#else
REG_COMM_ROUTE(1, 2, 0x02)
REG_COMM_ROUTE(2, 1, 0x01)
#endif /* PLECS_NODE_ADDR */

/**
 * @brief Echo a valid request and identify the addressed node through ACK source fields.
 * @param[in] p_pack Validated request supplied by the FRAME protocol parser.
 * @param[in] my_printf Output interface associated with the request's source link.
 */
static void loopback_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    section_packform_t ack = {0}; /* Direct response preserving command and payload bytes. */

    if ((p_pack == NULL) || /* No validated request is available. */
        (p_pack->is_ack != 0u) || /* ACK frames never generate another ACK. */
        ((p_pack->len > 0u) && /* A non-empty request claims payload bytes. */
         (p_pack->p_data == NULL))) /* The claimed payload is not readable. */
    {
        return;
    }

    ++route_state.loopback_count;
    ack.src = p_pack->dst;
    ack.d_src = p_pack->d_dst;
    ack.dst = p_pack->src;
    ack.d_dst = p_pack->d_src;
    ack.cmd_set = p_pack->cmd_set;
    ack.cmd_word = p_pack->cmd_word;
    ack.is_ack = 1u;
    ack.len = p_pack->len;
    ack.p_data = p_pack->p_data;
    comm_send_data(&ack, my_printf);
}

REG_COMM(0x30, 0x01, loopback_act)

/**
 * @brief Refresh visible connection state and PLECS outputs for this node.
 */
static void route_bridge_sample(void)
{
    route_state.peer_connected = (uint32_t)peer_tcp_link_is_connected();
    route_state.frame_connected = (uint32_t)frame_tcp_server_is_connected();

    plecs_set_output(PLECS_OUTPUT_NODE_VALUE, (float)route_state.node_value);
    plecs_set_output(PLECS_OUTPUT_PEER_CONNECTED, (float)route_state.peer_connected);
}

REG_INTERRUPT(0, route_bridge_sample)

void route_bridge_state_reset(void)
{
    route_state.node_addr = (uint32_t)PLECS_NODE_ADDR;
    route_state.node_value = (uint32_t)PLECS_NODE_ADDR;
    route_state.loopback_count = 0u;
    route_state.peer_connected = 0u;
    route_state.frame_connected = 0u;
}
