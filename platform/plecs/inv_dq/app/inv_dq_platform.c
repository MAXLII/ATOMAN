// SPDX-License-Identifier: MIT
/**
 * @file    inv_dq_platform.c
 * @brief   PLECS inverter DQ platform lifecycle adapter.
 * @details
 *          This file is part of the base PLECS inverter DQ project.
 *
 *          Module responsibilities:
 *          - Bind the shared FRAME TCP server to the PLECS DLL lifecycle
 *          - Serialize inverter simulation callbacks with protocol dispatch
 *          - Stop the communication worker when the simulation terminates
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Shell registrations remain owned by app.c
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
#include "frame_tcp_server.h"
#include "plecs.h"

void plecs_platform_start(void)
{
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
