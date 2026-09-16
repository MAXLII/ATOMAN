// SPDX-License-Identifier: MIT
/**
 * @file    frame_bridge_platform.c
 * @brief   PLECS FRAME Bridge platform lifecycle adapter.
 * @details
 *          This file is part of the base PLECS FRAME Bridge project.
 *
 *          Module responsibilities:
 *          - Reset FRAME Bridge application state before scheduled communication runs
 *          - Let SECTION initialize and poll the shared simulation BSP
 *          - Keep all application and protocol dispatch on the simulation thread
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - TCP transport implementation resides in code/sim/comm
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
#include "plecs.h"

void plecs_platform_start(void)
{
    frame_bridge_state_reset();
}
