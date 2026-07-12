// SPDX-License-Identifier: MIT
/**
 * @file    bsp_gpio.h
 * @brief   APM32 GPIO BSP interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Define the GPIO table used by the shared AC demo interface
 *          - Declare table-driven GPIO initialization and bit access APIs
 *          - Keep the public BSP shape aligned with other platform ports
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __BSP_GPIO_H
#define __BSP_GPIO_H

#include "apm32f402_403.h"

#include <stdint.h>

#define GPIO_REG_PARM(name, gpx, _pin, _mode, _speed, _def_lv) \
    [name] = {                                                 \
        .periph = RCM_APB2_PERIPH_##gpx,                       \
        .port = gpx,                                           \
        .pin = GPIO_PIN_##_pin,                                \
        .mode = GPIO_MODE_##_mode,                             \
        .speed = GPIO_SPEED_##_speed,                          \
        .def_lv = (_def_lv),                                   \
        .bsp_gpio_table = (name),                              \
    }

typedef enum
{
    LED1,
    LED2,
    LED3,
    LED4,
    GPIO_TABLE_MAX,
} bsp_gpio_table_e;

typedef struct
{
    uint32_t periph;
    GPIO_T *port;
    uint16_t pin;
    uint32_t mode;
    uint32_t speed;
    uint8_t def_lv;
    bsp_gpio_table_e bsp_gpio_table;
} bsp_gpio_parm_t;

void bsp_gpio_init(void);
void bsp_gpio_set_bit(bsp_gpio_table_e num, uint8_t val);
void bsp_gpio_get_bit(bsp_gpio_table_e num, uint8_t *val);

#endif
