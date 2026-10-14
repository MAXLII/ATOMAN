// SPDX-License-Identifier: MIT
/**
 * @file bsp_clk.h
 * @brief Board clock interface.
 * @details
 *          This file is part of the digital power framework project.
 *          Module responsibilities:
 *          - Initialize the 8 MHz HSE / 72 MHz system clock
 *          Design notes:
 *          - C11 compatible; no dynamic allocation in platform drivers
 *          - Hardware access belongs to the BSP; framework bindings belong to src
 * @author Max.Li
 * @date 2026-10-14
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li.
 *          All rights reserved.
 *          This file is licensed under the MIT License.
 *          See the LICENSE file in the project root for full license text.
 */
#ifndef BSP_CLK_H
#define BSP_CLK_H
/** @brief Verify the vendor startup configured HSE x9 and legal APB clocks. */
void bsp_clk_init(void);
#endif /* BSP_CLK_H */
