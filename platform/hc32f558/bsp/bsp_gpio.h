// SPDX-License-Identifier: MIT
/**
 * @file    bsp_gpio.h
 * @brief   HC32F558 GPIO BSP interface.
 * @details
 *          This file is part of the HC32F558 AC project.
 *
 *          Module responsibilities:
 *          - Define logical GPIO names used by the AC interface layer
 *          - Provide board pin configuration table types
 *          - Export simple set/get helpers for relay and debug GPIOs
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - GPIO access is safe for task context
 *          - Hardware access is abstracted through HC32 LL GPIO APIs
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

#ifndef BSP_GPIO_H
#define BSP_GPIO_H

#include <stdint.h>
#include "hc32_ll.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BSP_GPIO_LED0 = 0,
    BSP_GPIO_LED1,
    BSP_GPIO_LED2,
    BSP_GPIO_LED3,
    BSP_GPIO_MAIN_RLY,
    BSP_GPIO_SS_RLY,
    BSP_GPIO_AC_IN_RLY,
    BSP_GPIO_AC_OUT_RLY,
    BSP_GPIO_TEST1,
    BSP_GPIO_MAX,

    LED1 = BSP_GPIO_MAIN_RLY,
    LED2 = BSP_GPIO_SS_RLY,
    LED3 = BSP_GPIO_AC_IN_RLY,
    LED4 = BSP_GPIO_AC_OUT_RLY,
} bsp_gpio_table_e;

typedef enum {
    BSP_GPIO_MODE_OUT_PP = 0,
    BSP_GPIO_MODE_OUT_OD,
    BSP_GPIO_MODE_IN_PD,
    BSP_GPIO_MODE_IN_PU,
} bsp_gpio_mode_e;

typedef struct {
    bsp_gpio_table_e table;
    uint8_t port;
    uint32_t pin;
    bsp_gpio_mode_e mode;
    uint8_t def_level;
} bsp_gpio_param_t;

extern const bsp_gpio_param_t bsp_gpio_param_table[BSP_GPIO_MAX];

void bsp_gpio_init(void);
void bsp_gpio_set_bit(bsp_gpio_table_e num, uint8_t val);
void bsp_gpio_get_bit(bsp_gpio_table_e num, uint8_t *val);

#ifdef __cplusplus
}
#endif

#endif /* BSP_GPIO_H */
