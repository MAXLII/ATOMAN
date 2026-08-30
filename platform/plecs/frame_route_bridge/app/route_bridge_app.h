// SPDX-License-Identifier: MIT
/**
 * @file    route_bridge_app.h
 * @brief   PLECS FRAME route-bridge application interface.
 * @details
 *          This file is part of the base PLECS FRAME route-bridge project.
 *
 *          Module responsibilities:
 *          - Declare reset of node-specific parameter and protocol state
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Node identity is selected through the PLECS_NODE_ADDR build definition
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
#ifndef ROUTE_BRIDGE_APP_H
#define ROUTE_BRIDGE_APP_H

void route_bridge_state_reset(void);

#endif /* ROUTE_BRIDGE_APP_H */
