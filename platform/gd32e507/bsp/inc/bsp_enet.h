// SPDX-License-Identifier: MIT
/**
 * @file    bsp_enet.h
 * @brief   GD32E507Z-EVAL Ethernet BSP interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Expose RMII MAC and DP83848 initialization
 *          - Expose PHY link-state and receive-pending queries
 *          - Publish low-level initialization diagnostics
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The ENET path is polled from background context
 *          - Hardware access uses the GD32E50x standard peripheral library
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

#ifndef BSP_ENET_H
#define BSP_ENET_H

#include <stdint.h>

extern volatile uint32_t g_bsp_enet_init_attempt_count;
extern volatile uint32_t g_bsp_enet_init_error_count;
extern volatile uint32_t g_bsp_enet_phy_read_error_count;

uint8_t bsp_enet_init(void);
uint8_t bsp_enet_link_is_up(void);
uint8_t bsp_enet_rx_frame_pending(void);

#endif /* BSP_ENET_H */
