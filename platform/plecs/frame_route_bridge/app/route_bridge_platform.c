// SPDX-License-Identifier: MIT
/**
 * @file    route_bridge_platform.c
 * @brief   PLECS FRAME route-bridge lifecycle adapter.
 * @details
 *          This file is part of the base PLECS FRAME route-bridge project.
 *
 *          Module responsibilities:
 *          - Reset node application state for each simulation run
 *          - Let SECTION initialize and poll Frame-facing and peer links
 *          - Leave transport cleanup to the common host termination callback
 *          - Keep protocol dispatch in the serialized SECTION task context
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Each node owns one Frame server while both nodes own one peer link
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
#include "plecs.h"

void plecs_platform_start(void)
{
    route_bridge_state_reset();
}
