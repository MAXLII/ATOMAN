// SPDX-License-Identifier: MIT
/**
 * @file    plecs_port.h
 * @brief   PLECS FRAME bridge signal-port definition.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Define the single PLECS input used by the bridge demonstration
 *          - Define the single PLECS output calculated from FRAME-adjustable parameters
 *          - Keep DLL signal counts synchronized with the PLECS model
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The signal path runs in the PLECS simulation callback
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
#ifndef PLECS_FRAME_BRIDGE_PORT_H
#define PLECS_FRAME_BRIDGE_PORT_H

typedef enum
{
    PLECS_INPUT_SIGNAL = 0,
    PLECS_INPUT_MAX
} PLECS_INPUT_E;

typedef enum
{
    PLECS_OUTPUT_SIGNAL = 0,
    PLECS_OUTPUT_MAX
} PLECS_OUTPUT_E;

#endif /* PLECS_FRAME_BRIDGE_PORT_H */
