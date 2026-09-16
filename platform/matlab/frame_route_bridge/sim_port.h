// SPDX-License-Identifier: MIT
/**
 * @file    sim_port.h
 * @brief   SIM FRAME route-bridge signal-port definition.
 * @details
 *          This file is part of the base SIM FRAME route-bridge project.
 *
 *          Module responsibilities:
 *          - Define one unused input that keeps both DLL Block layouts identical
 *          - Expose node value and peer-link state as SIM outputs
 *          - Keep the node 0x02 and node 0x03 DLL interfaces interchangeable
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Signal values are updated from the SIM simulation callback
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
#ifndef SIM_FRAME_ROUTE_BRIDGE_PORT_H
#define SIM_FRAME_ROUTE_BRIDGE_PORT_H

typedef enum
{
    SIM_INPUT_UNUSED = 0, /**< Model input retained for a visible DLL Block connection. */
    SIM_INPUT_MAX
} SIM_INPUT_E;

typedef enum
{
    SIM_OUTPUT_NODE_VALUE = 0, /**< Current FRAME-writable value owned by this node. */
    SIM_OUTPUT_PEER_CONNECTED, /**< 1 when the internal node link is connected. */
    SIM_OUTPUT_MAX
} SIM_OUTPUT_E;

#define SIM_INPUT_NUM SIM_INPUT_MAX
#define SIM_OUTPUT_NUM SIM_OUTPUT_MAX
#define SIM_SAMPLE_TIME_S (1.0e-4)
#define SIM_TICK_UNIT_US (100U)
#define SIM_TICK_STEP_S ((double)SIM_TICK_UNIT_US * 1.0e-6)
#endif /* SIM_FRAME_ROUTE_BRIDGE_PORT_H */
