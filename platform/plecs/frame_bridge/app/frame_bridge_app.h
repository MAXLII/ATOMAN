// SPDX-License-Identifier: MIT
/**
 * @file    frame_bridge_app.h
 * @brief   PLECS FRAME bridge simulation-state interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Reset PLECS simulation state before the FRAME TCP service starts
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Shell variables are registered by the matching implementation file
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
#ifndef FRAME_BRIDGE_APP_H
#define FRAME_BRIDGE_APP_H

void frame_bridge_state_reset(void);

#endif /* FRAME_BRIDGE_APP_H */
