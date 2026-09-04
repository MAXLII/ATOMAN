// SPDX-License-Identifier: MIT
/**
 * @file    demo_protocol.h
 * @brief   Demo FRAME protocol contract.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Define the demo loopback payload layout
 *          - Define demo command-set and command-word values
 *          - Share the wire contract with protocol probes
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - New wire fields are appended at the structure tail
 *          - Receivers bound copies by the received payload length
 *
 * @author  Max.Li
 * @date    2026-09-05
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef DEMO_PROTOCOL_H
#define DEMO_PROTOCOL_H

#include <stdint.h>

typedef struct
{
    uint32_t counter;
    uint8_t led_mask;
    int16_t temperature_x10;
} demo_comm_frame_t;

#define DEMO_CMD_SET_LOOPBACK 0x30u
#define DEMO_CMD_WORD_LOOPBACK 0x01u
#define DEMO_CMD_SET_FRAME_LOOPBACK 0x01u
#define DEMO_CMD_WORD_FRAME_LOOPBACK 0x17u
#define DEMO_CMD_SET_CONTROL 0x30u
#define DEMO_CMD_WORD_CONTROL 0x02u

#endif /* DEMO_PROTOCOL_H */
