// SPDX-License-Identifier: MIT
/**
 * @file bsp_gpio.h
 * @brief Board indicator interface.
 * @details
 *          This file is part of the digital power framework project.
 *          Module responsibilities:
 *          - Operate the active-low PB3 DS0 LED without touching SWD pins
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
#ifndef BSP_GPIO_H
#define BSP_GPIO_H
void bsp_gpio_init(void);
void bsp_gpio_led_toggle(void);
#endif /* BSP_GPIO_H */
