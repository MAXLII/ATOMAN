// SPDX-License-Identifier: MIT
/**
 * @file    bsp_gpio.h
 * @brief   GD32E507Z-EVAL GPIO BSP interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Name the 4 evaluation-board LEDs
 *          - Expose logical GPIO read and write operations
 *          - Keep board pin assignments outside shared interface code
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - GPIO functions are background-safe and use atomic set/reset registers
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

#ifndef BSP_GPIO_H
#define BSP_GPIO_H

#include <stdint.h>

typedef enum
{
    LED1 = 0, /* Evaluation-board LED1 on PG10. */
    LED2,     /* Evaluation-board LED2 on PG11. */
    LED3,     /* Evaluation-board LED3 on PG12. */
    LED4,     /* Evaluation-board LED4 on PG13. */
    BSP_GPIO_TEST1 = LED4,
    GPIO_TABLE_MAX
} bsp_gpio_table_e;

void bsp_gpio_init(void);
void bsp_gpio_set_bit(bsp_gpio_table_e number, uint8_t value);
void bsp_gpio_get_bit(bsp_gpio_table_e number, uint8_t *p_value);

#endif /* BSP_GPIO_H */
