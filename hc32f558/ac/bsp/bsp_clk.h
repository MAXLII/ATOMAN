// SPDX-License-Identifier: MIT
/**
 * @file    bsp_clk.h
 * @brief   HC32F558 system clock BSP interface.
 * @details
 *          This file is part of the HC32F558 project.
 *
 *          Module responsibilities:
 *          - Expose the board-level system clock initialization entry
 *          - Provide compile-time clock frequency constants used by the BSP
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path is not required; initialization runs before scheduler start
 *          - Hardware access is abstracted through the HC32 LL driver
 *
 * @author  Max.Li
 * @date    2026-06-06
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef BSP_CLK_H
#define BSP_CLK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_CLK_SYSCLK_HZ (240000000UL)
#define BSP_CLK_HCLK_HZ   (240000000UL)
#define BSP_CLK_PCLK0_HZ  (240000000UL)
#define BSP_CLK_PCLK1_HZ  (120000000UL)
#define BSP_CLK_PCLK2_HZ  (60000000UL)
#define BSP_CLK_PCLK3_HZ  (60000000UL)
#define BSP_CLK_PCLK4_HZ  (120000000UL)

int32_t bsp_clk_init(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_CLK_H */
