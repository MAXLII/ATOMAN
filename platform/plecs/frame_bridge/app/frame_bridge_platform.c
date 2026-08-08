// SPDX-License-Identifier: MIT
/**
 * @file    frame_bridge_platform.c
 * @brief   PLECS FRAME Bridge platform lifecycle adapter.
 * @details
 *          This file is part of the base PLECS FRAME Bridge project.
 *
 *          Module responsibilities:
 *          - Reset FRAME Bridge application state before TCP communication starts
 *          - Bind the shared PLECS TCP server to the DLL lifecycle callbacks
 *          - Serialize simulation callbacks with FRAME protocol dispatch
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - TCP transport implementation remains in platform/plecs/common
 *          - Hardware access is not used by this simulation project
 *
 * @author  Max.Li
 * @date    2026-08-08
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "frame_bridge_app.h"

#include "frame_tcp_server.h"
#include "plecs.h"

void plecs_platform_start(void)
{
    frame_bridge_state_reset();
    frame_tcp_server_start();
}

void plecs_platform_terminate(void)
{
    frame_tcp_server_stop();
}

void plecs_platform_dispatch_enter(void)
{
    frame_tcp_server_dispatch_enter();
}

void plecs_platform_dispatch_exit(void)
{
    frame_tcp_server_dispatch_exit();
}
