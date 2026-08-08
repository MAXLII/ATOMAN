// SPDX-License-Identifier: MIT
/**
 * @file    bsp_gpio.c
 * @brief   GD32E507Z-EVAL GPIO BSP implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Configure PG10 through PG13 as LED outputs
 *          - Translate logical LED numbers into board GPIO pins
 *          - Toggle LED1 as a visible 500 ms firmware heartbeat
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - LED writes use atomic GPIO set/reset operations
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

#include "bsp_gpio.h"

#include "gd32e50x.h"
#include "section.h"

static const uint32_t led_pins[GPIO_TABLE_MAX] = { /* Board LED pin map indexed by bsp_gpio_table_e. */
    GPIO_PIN_10,
    GPIO_PIN_11,
    GPIO_PIN_12,
    GPIO_PIN_13,
};

static uint8_t heartbeat_state = 0u; /* Current LED1 heartbeat output state. */

void bsp_gpio_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOG);
    gpio_init(GPIOG,
              GPIO_MODE_OUT_PP,
              GPIO_OSPEED_50MHZ,
              GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13);
    gpio_bit_reset(GPIOG, GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13);
}

REG_INIT(0, bsp_gpio_init)

void bsp_gpio_set_bit(bsp_gpio_table_e number, uint8_t value)
{
    if ((uint32_t)number >= (uint32_t)GPIO_TABLE_MAX)
    {
        return;
    }

    if (value == 1u)
    {
        gpio_bit_set(GPIOG, led_pins[number]);
    }
    else
    {
        gpio_bit_reset(GPIOG, led_pins[number]);
    }
}

void bsp_gpio_get_bit(bsp_gpio_table_e number, uint8_t *p_value)
{
    if (((uint32_t)number >= (uint32_t)GPIO_TABLE_MAX) || /* Logical GPIO number is outside the board map. */
        (p_value == NULL))                               /* The caller did not provide output storage. */
    {
        return;
    }

    *p_value = (uint8_t)gpio_output_bit_get(GPIOG, led_pins[number]);
}

static void heartbeat_task(void)
{
    heartbeat_state ^= 1u;
    bsp_gpio_set_bit(LED1, heartbeat_state);
}

REG_TASK_MS(500, heartbeat_task)
