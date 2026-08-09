// SPDX-License-Identifier: MIT
/**
 * @file    frame_tcp_server.h
 * @brief   Shared FRAME TCP transport for PLECS projects.
 * @details
 *          This file is part of the base PLECS common platform.
 *
 *          Module responsibilities:
 *          - Declare the TCP server port used by FRAME
 *          - Advertise the active PLECS simulation through UDP device discovery
 *          - Expose transport lifecycle operations for PLECS project adapters
 *          - Serialize simulation dispatch and FRAME protocol callbacks
 *          - Keep Windows socket details private to the transport implementation
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Socket receive and protocol dispatch run in one worker thread
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
#ifndef FRAME_TCP_SERVER_H
#define FRAME_TCP_SERVER_H

#define FRAME_TCP_SERVER_PORT (5000u)

void frame_tcp_server_start(void);
void frame_tcp_server_stop(void);
void frame_tcp_server_dispatch_enter(void);
void frame_tcp_server_dispatch_exit(void);

#endif /* FRAME_TCP_SERVER_H */
