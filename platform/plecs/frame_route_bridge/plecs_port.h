// SPDX-License-Identifier: MIT
/**
 * @file    plecs_port.h
 * @brief   PLECS FRAME route-bridge signal-port definition.
 * @details
 *          This file is part of the base PLECS FRAME route-bridge project.
 *
 *          Module responsibilities:
 *          - Define one unused input that keeps both DLL Block layouts identical
 *          - Expose node value and peer-link state as PLECS outputs
 *          - Keep the node 0x02 and node 0x03 DLL interfaces interchangeable
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Signal values are updated from the PLECS simulation callback
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
#ifndef PLECS_FRAME_ROUTE_BRIDGE_PORT_H
#define PLECS_FRAME_ROUTE_BRIDGE_PORT_H

typedef enum
{
    PLECS_INPUT_UNUSED = 0, /**< Model input retained for a visible DLL Block connection. */
    PLECS_INPUT_MAX
} PLECS_INPUT_E;

typedef enum
{
    PLECS_OUTPUT_NODE_VALUE = 0, /**< Current FRAME-writable value owned by this node. */
    PLECS_OUTPUT_PEER_CONNECTED, /**< 1 when the internal node link is connected. */
    PLECS_OUTPUT_MAX
} PLECS_OUTPUT_E;

#endif /* PLECS_FRAME_ROUTE_BRIDGE_PORT_H */
