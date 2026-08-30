// SPDX-License-Identifier: MIT
/**
 * @file    route_bridge_platform.c
 * @brief   PLECS FRAME route-bridge lifecycle adapter.
 * @details
 *          This file is part of the base PLECS FRAME route-bridge project.
 *
 *          Module responsibilities:
 *          - Initialize the shared dispatch lock before any network worker starts
 *          - Start Frame-facing and peer-facing links required by each node role
 *          - Stop transport workers before destroying shared synchronization state
 *          - Serialize PLECS scheduling with both TCP protocol dispatch paths
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Node 0x02 owns the Frame server while both nodes own one peer link
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

#include "frame_tcp_server.h"
#include "peer_tcp_link.h"
#include "plecs.h"
#include "plecs_dispatch_lock.h"

#ifndef PLECS_NODE_ADDR
#error "PLECS_NODE_ADDR must be defined by the node DLL target"
#endif

void plecs_platform_start(void)
{
    plecs_dispatch_lock_start();
    route_bridge_state_reset();
#if (PLECS_NODE_ADDR == 0x02)
    frame_tcp_server_start();
#endif /* PLECS_NODE_ADDR */
    peer_tcp_link_start();
}

void plecs_platform_terminate(void)
{
    peer_tcp_link_stop();
#if (PLECS_NODE_ADDR == 0x02)
    frame_tcp_server_stop();
#endif /* PLECS_NODE_ADDR */
    plecs_dispatch_lock_stop();
}

void plecs_platform_dispatch_enter(void)
{
    plecs_dispatch_lock_enter();
}

void plecs_platform_dispatch_exit(void)
{
    plecs_dispatch_lock_exit();
}
